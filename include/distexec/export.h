
/*
 * Copyright (C) 2016-2017 Konrad Lother <k@hiddenbox.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */






#ifndef DISTEXEC_EXPORT_H
#define DISTEXEC_EXPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32)
#if defined(DISTEXEC_DLL_H)
#define EXPORT __declspec(dllexport) __cdecl
#else
#define EXPORT __declspec(dllimport) __cdecl
#endif
#else
#define EXPORT
#endif

#ifdef __cplusplus
}
#endif

#endif
