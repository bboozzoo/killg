/*
* Copyright (C) 2008  Maciek Borzecki
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef __LOGGER_H__
#define __LOGGER_H__
#include <iostream>

#ifdef LOG_ENABLE
#define LOG(__lvl, __x) \
    do { \
        std::cerr << __FILE__ << ":" << __LINE__ << " " << #__lvl << ": " << __x << std::endl; \
    } while(0)
#else
#define LOG(__lvl, __x)
#endif

#define LOG_ERR(__x) LOG(ERR, __x)
#define LOG_WARN(__x) LOG(WARN, __x)
#define LOG_INFO(__x) LOG(INFO, __x)

#endif /* __LOGGER_H__ */
