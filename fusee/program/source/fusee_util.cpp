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
#include "fusee_util.hpp"

namespace ams::nxboot {

    u32 ParseHexInteger(const char *s) {
        u32 x = 0;
        if (s[0] == '0' && s[1] == 'x') {
            s += 2;
        }

        while (true) {
            const char c = *(s++);

            if (c == '\x00') {
                return x;
            } else {
                x <<= 4;

                if ('0' <= c && c <= '9') {
                    x |= (c - '0');
                } else if ('a' <= c && c <= 'f') {
                    x |= (c - 'a') + 10;
                } else if ('A' <= c && c <= 'F') {
                    x |= (c - 'A') + 10;
                }
            }
        }
    }

    u32 ParseDecimalInteger(const char *s) {
        u32 x = 0;
        while (true) {
            const char c = *(s++);

            if (c == '\x00') {
                return x;
            } else {
                x *= 10;

                if ('0' <= c && c <= '9') {
                    x += c - '0';
                }
            }
        }
    }
}
