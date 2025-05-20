/*
 * Copyright (c) Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <exosphere.hpp>
#include "fusee_fs_storage.hpp"

namespace ams::fs {

        void MultiFileStorage::EnsureFile(int id) {
            if (!m_open[id]) {
                /* Update path. */
                m_base_path[m_file_path_ofs + 1] = '0' + (id % 10);
                m_base_path[m_file_path_ofs + 0] = '0' + (id / 10);

                /* Open new file. */
                const Result result = fs::OpenFile(m_handles + id, m_base_path, fs::OpenMode_Read);
                if (R_FAILED(result)) {
                    nxboot::ShowFatalError("Failed to open part %02d (%s): 0x%08" PRIx32 "!\n", id, m_base_path, result.GetValue());
                }

                m_open[id] = true;
            }
        }

        MultiFileStorage::MultiFileStorage(const char *base_path, fs::OpenMode mode) {
            util::Strlcpy(m_base_path, base_path, sizeof(m_base_path) - 2);
            m_file_path_ofs = strlen(m_base_path);

            for (size_t i = 0; i < util::size(m_handles); i++) {
                m_open[i] = false;
            }

            std::memcpy(m_base_path + m_file_path_ofs, "00", 3);
            Result r;
            if (R_FAILED(r = fs::OpenFile(std::addressof(m_handles[0]), m_base_path, mode))) {
                nxboot::ShowFatalError("Failed to open part %02d (%s): 0x%08" PRIx32 "!\n", 0, m_base_path, r.GetValue());
            }
            
            if (R_FAILED(r = fs::GetFileSize(&m_file_size, m_handles[0]))) {
                nxboot::ShowFatalError("Failed to get size %02d (%s): 0x%08" PRIx32 "!\n", 0, m_base_path, r.GetValue());
            }
            m_open[0] = true;
        }

        Result MultiFileStorage::Read(s64 offset, void *buffer, size_t size) {
            int file   = offset / m_file_size;
            s64 subofs = offset % m_file_size;

            u8 *cur_dst = static_cast<u8 *>(buffer);

            for (/* ... */; size > 0; ++file) {
                /* Ensure the current file is open. */
                EnsureFile(file);

                /* Perform the current read. */
                const size_t cur_size = std::min<size_t>(m_file_size - subofs, size);
                R_TRY(fs::ReadFile(m_handles[file], subofs, cur_dst, cur_size));

                /* Advance. */
                cur_dst += cur_size;
                size -= cur_size;
                subofs = 0;
            }

            R_SUCCEED();
        }

        Result MultiFileStorage::Flush() {
            R_THROW(fs::ResultUnsupportedOperation());
        }

        Result MultiFileStorage::GetSize(s64 *out) {
            R_THROW(fs::ResultUnsupportedOperation());
        }

        Result MultiFileStorage::Write(s64 offset, const void *buffer, size_t size) {
            int file   = offset / m_file_size;
            s64 subofs = offset % m_file_size;

            const u8 *cur_src = static_cast<const u8 *>(buffer);

            for (/* ... */; size > 0; ++file) {
                /* Ensure the current file is open. */
                EnsureFile(file);

                /* Perform the current read. */
                const size_t cur_size = std::min<size_t>(m_file_size - subofs, size);
                R_TRY(fs::WriteFile(m_handles[file], subofs, cur_src, cur_size, fs::WriteOption::Flush));

                /* Advance. */
                cur_src += cur_size;
                size -= cur_size;
                subofs = 0;
            }

            R_SUCCEED();
        }

        Result MultiFileStorage::SetSize(s64 size) {
            R_THROW(fs::ResultUnsupportedOperation());
        }

}
