/*
 * NX-GZIP compression accelerator user library
 * implementing zlib compression library interfaces
 *
 * Copyright (C) IBM Corporation, 2011-2017
 *
 * Licenses for GPLv2 and Apache v2.0:
 *
 * GPLv2:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 * Apache v2.0:
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Authors: Xiao Hua Zeng <zengxhsh@cn.ibm.com>
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <endian.h>
#include <pthread.h>
#include <signal.h>
#include <zlib.h>
#include <dlfcn.h>
#include "nx_dbg.h"
#include "nx_zlib.h"

#define DEFAULT_ZLIB_PATH "/lib64/libz.so.1"


static void *sw_handler = NULL;

typedef void * __attribute__ ((__may_alias__)) pvoid_t;

#define register_sym(name)						\
	do {								\
		dlerror();    /* Clear any existing error */		\
		*(pvoid_t *)(&p_##name) = dlsym(sw_handler, #name);		\
		if ((error = dlerror()) != NULL) {			\
			prt_err("%s\n", error);			\
		}							\
	} while (0)

#define check_sym(name, rc)						\
	do {								\
		if ((name) == NULL) {					\
			prt_err("%s not loadable, consider using a "	\
			       "newer libz version.\n", #name);		\
			return (rc);					\
		}							\
	} while (0)

static const char * (* p_zlibVersion)(void);
const char *s_zlibVersion(void)
{
	check_sym(p_zlibVersion, NULL);
	return (* p_zlibVersion)();
}

static int (* p_deflateInit_)(z_streamp strm, int level, const char* version, int stream_size);
int s_deflateInit_(z_streamp strm, int level, const char* version, int stream_size)
{
	int rc;

	check_sym(p_deflateInit_, Z_STREAM_ERROR);
        rc = (* p_deflateInit_)(strm, level, version, stream_size);
        return rc;
}

static int (* p_deflateInit2_)(z_streamp strm, int level, int method,
			int windowBits, int memLevel, int strategy,
			const char *version, int stream_size);
int s_deflateInit2_(z_streamp strm, int level, int method,
		    int windowBits, int memLevel, int strategy,
		    const char *version, int stream_size)
{
	int rc;

	check_sym(p_deflateInit2_, Z_STREAM_ERROR);
	rc = (* p_deflateInit2_)(strm, level, method, windowBits, memLevel,
				 strategy, version, stream_size);
	return rc;
}

static uLong (* p_deflateBound)(z_streamp strm, uLong sourceLen);
uLong s_deflateBound(z_streamp strm, uLong sourceLen)
{
	check_sym(p_deflateBound, Z_STREAM_ERROR);
	return (* p_deflateBound)(strm, sourceLen);
}

static int (* p_deflateReset)(z_streamp strm);
int s_deflateReset(z_streamp strm)
{
	check_sym(p_deflateReset, Z_STREAM_ERROR);
	return (* p_deflateReset)(strm);
}

static int (* p_deflateResetKeep)(z_streamp strm);
int s_deflateResetKeep(z_streamp strm)
{
	check_sym(p_deflateResetKeep, Z_STREAM_ERROR);
	return (* p_deflateResetKeep)(strm);
}

static int (* p_deflateSetDictionary)(z_streamp strm, const Bytef *dictionary,
				      uInt  dictLength);
int s_deflateSetDictionary(z_streamp strm, const Bytef *dictionary,
			   uInt  dictLength)
{
	check_sym(p_deflateSetDictionary, Z_STREAM_ERROR);
	return (* p_deflateSetDictionary)(strm, dictionary, dictLength);
}

static int (* p_deflateSetHeader)(z_streamp strm, gz_headerp head);
int s_deflateSetHeader(z_streamp strm, gz_headerp head)
{
	check_sym(p_deflateSetHeader, Z_STREAM_ERROR);
	return p_deflateSetHeader(strm, head);
}


static int (* p_deflate)(z_streamp strm, int flush);
int s_deflate(z_streamp strm, int flush)
{
	check_sym(p_deflate, Z_STREAM_ERROR);
	return (* p_deflate)(strm, flush);
}

static int (* p_deflateEnd)(z_streamp strm);
int s_deflateEnd(z_streamp strm)
{
	check_sym(p_deflateEnd, Z_STREAM_ERROR);
	return (* p_deflateEnd)(strm);
}

static int (* p_uncompress)(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
int s_uncompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen)
{
	check_sym(p_uncompress, Z_STREAM_ERROR);
	return (* p_uncompress)(dest, destLen, source, sourceLen);
}


static int (* p_inflateInit_)(z_streamp strm, const char *version, int stream_size);
int s_inflateInit_(z_streamp strm, const char *version, int stream_size)
{
	int rc;
	check_sym(p_inflateInit_, Z_STREAM_ERROR);
	rc = (* p_inflateInit_)(strm, version, stream_size);
	return rc;
}

static int (* p_inflateInit2_)(z_streamp strm, int  windowBits,
			       const char *version, int stream_size);
int s_inflateInit2_(z_streamp strm, int  windowBits, const char *version,
		    int stream_size)
{
	int rc;

	check_sym(p_inflateInit2_, Z_STREAM_ERROR);
	rc = (* p_inflateInit2_)(strm, windowBits, version, stream_size);
	prt_info("p_inflateInit2_: return %d\n",rc);
	return rc;
}

static int (* p_inflateReset)(z_streamp strm);
int s_inflateReset(z_streamp strm)
{
	check_sym(p_inflateReset, Z_STREAM_ERROR);
	return (* p_inflateReset)(strm);
}

static int (* p_inflateReset2)(z_streamp strm, int windowBits);
int s_inflateReset2(z_streamp strm, int windowBits)
{
	check_sym(p_inflateReset2, Z_STREAM_ERROR);
	return (* p_inflateReset2)(strm, windowBits);
	
}

static int (*p_inflateResetKeep)(z_streamp strm);
int s_inflateResetKeep(z_streamp strm)
{
	check_sym(p_inflateResetKeep, Z_STREAM_ERROR);
	return (* p_inflateResetKeep)(strm);
}

static int (* p_inflateSetDictionary)(z_streamp strm, const Bytef *dictionary,
				 uInt  dictLength);
int s_inflateSetDictionary(z_streamp strm, const Bytef *dictionary,
			   uInt  dictLength)
{
	check_sym(p_inflateSetDictionary, Z_STREAM_ERROR);
	return (* p_inflateSetDictionary)(strm, dictionary, dictLength);
}

static int (* p_inflate)(z_streamp strm, int flush);
int s_inflate(z_streamp strm, int flush)
{
	check_sym(p_inflate, Z_STREAM_ERROR);
	return (* p_inflate)(strm, flush);
}

static int (* p_inflateEnd)(z_streamp strm);
int s_inflateEnd(z_streamp strm)
{
	check_sym(p_inflateEnd, Z_STREAM_ERROR);
	return (* p_inflateEnd)(strm);
}


static int (* p_compress)(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
int s_compress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen)
{
	check_sym(p_compress, Z_STREAM_ERROR);
	return (* p_compress)(dest, destLen, source, sourceLen);
}

static int (* p_compress2)(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level);
int s_compress2(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level)
{
	check_sym(p_compress2, Z_STREAM_ERROR);
	return (* p_compress2)(dest, destLen, source, sourceLen, level);
}

static int (* p_compressBound)(uLong sourceLen);
uLong s_compressBound(uLong sourceLen)
{
	check_sym(p_compressBound, Z_STREAM_ERROR);
	return (* p_compressBound)(sourceLen);
}


/*
 * Open the zlib dl and Register APIs 
 */
void sw_zlib_init(void)
{
	char *error;

	sw_handler = dlopen(DEFAULT_ZLIB_PATH, RTLD_LAZY);
	if(sw_handler == NULL) {
		prt_err(" %s\n", dlerror());
		return;
	}
	
	register_sym(zlibVersion);
	
	register_sym(deflateInit_);
	register_sym(deflateInit2_);
	register_sym(deflateReset);
	register_sym(deflateResetKeep);
	register_sym(deflateBound);
	register_sym(deflateSetHeader);
	register_sym(deflate);
	register_sym(deflateEnd);
	register_sym(uncompress);

	register_sym(inflateInit_);
	register_sym(inflateInit2_);
	register_sym(inflateReset);
	register_sym(inflateReset2);
	register_sym(inflateResetKeep);
	register_sym(inflateSetDictionary);
	register_sym(inflate);
	register_sym(inflateEnd);
	register_sym(compress);
	register_sym(compress2);
		
	prt_info("software zlib version:%s\n",s_zlibVersion());
	return;
}

/*
 * Close zlib dl
 */
void sw_zlib_close(void)
{
	if(sw_handler != NULL){
		dlclose(sw_handler);
	}
	
	return;
}
