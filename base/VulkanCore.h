/*
 *
 ******************************************************************************
 *    Copyright [2024] [YongSong]
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 ******************************************************************************
 *
 */

#ifndef VULKAN_CORE_HEADER
#define VULKAN_CORE_HEADER

#pragma once

#if defined(EXPORT_LIB)
#ifdef __GNUC__
#define DVAPI_ATTR __attribute__((dllexport))
#define DVAPI_CALL __attribute__((stdcall))
#endif
#ifdef _MSC_VER
#define DVAPI_ATTR __declspec(dllexport)
#define DVAPI_CALL __declspec(stdcall)
#endif
#elif defined(NO_EXPORT_IMPORT)
#ifdef __GNUC__
#define DVAPI_ATTR
#define DVAPI_CALL
#endif
#ifdef _MSC_VER
#define DVAPI_ATTR
#define DVAPI_CALL
#endif
#elif defined(IMPORT_LIB)
#ifdef __GNUC__
#define DVAPI_ATTR __attribute__((dllimport))
#define DVAPI_CALL __attribute__((stdcall))
#endif
#ifdef _MSC_VER
#define DVAPI_ATTR __declspec(dllimport)
#define DVAPI_CALL __declspec(stdcall)
#endif
#endif

#endif
