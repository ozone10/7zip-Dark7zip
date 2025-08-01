## License
The license for use and distribution is the same as that of the original 7-Zip.  
Additionally, code in `DarkMode` folder is from [darkmodelib](https://github.com/ozone10/darkmodelib) which is licensed under the MIT License or the Mozilla Public License, version 2.0.  

## Dark mode source code (darkmodelib)

Copyright (c) 2024-2025 ozone10

Mozilla Public License Version 2.0
--------------------

    Copyright (c) 2025 ozone10
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.

MIT License
--------------------

    Copyright (c) 2024-2025 ozone10

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

## 7-Zip source code

### License for use and distribution


7-Zip Copyright (C) 1999-2025 Igor Pavlov.

The licenses for files are:

- CPP/7zip/Compress/Rar* files: the "GNU LGPL" with "unRAR license restriction"
- CPP/7zip/Compress/LzfseDecoder.cpp: the "BSD 3-clause License"
- C/ZstdDec.c: the "BSD 3-clause License"
- C/Xxh64.c: the "BSD 2-clause License"
- Some files are "public domain" files, if "public domain" status is stated in source file.
- the "GNU LGPL" for all other files. If there is no license information in
    some source file, that file is under the "GNU LGPL".

The "GNU LGPL" with "unRAR license restriction" means that you must follow both 
"GNU LGPL" rules and "unRAR license restriction" rules.




GNU LGPL information
--------------------

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, 
    you can get a copy of the GNU Lesser General Public License from
    http://www.gnu.org/




BSD 3-clause License in 7-Zip code
----------------------------------

The "BSD 3-clause License" is used for the following code in 7z.dll
1. LZFSE data decompression.
        CPP/7zip/Compress/LzfseDecoder.cpp.
    That code was derived from the code in the "LZFSE compression library" developed by Apple Inc,
    that also uses the "BSD 3-clause License".
2. ZSTD data decompression.
        C/ZstdDec.c
    that code was developed using original zstd decoder code as reference code.
    The original zstd decoder code was developed by Facebook Inc,
    that also uses the "BSD 3-clause License".

Copyright (c) 2015-2016, Apple Inc. All rights reserved.  
Copyright (c) Facebook, Inc. All rights reserved.  
Copyright (c) 2023-2025 Igor Pavlov.  

Text of the "BSD 3-clause License"
----------------------------------

        Redistribution and use in source and binary forms, with or without modification,
        are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this
        list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may
        be used to endorse or promote products derived from this software without
        specific prior written permission.

        THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
        ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
        WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
        DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
        ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
        (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
        LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
        ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
        (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
        SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

---




BSD 2-clause License in 7-Zip code
----------------------------------

  The "BSD 2-clause License" is used for the XXH64 code in 7-Zip.
    C/Xxh64.c

  XXH64 code in 7-Zip was derived from the original XXH64 code developed by Yann Collet.

  Copyright (c) 2012-2021 Yann Collet.  
  Copyright (c) 2023-2025 Igor Pavlov.

Text of the "BSD 2-clause License"
----------------------------------

        Redistribution and use in source and binary forms, with or without modification,
        are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this
        list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.

        THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
        ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
        WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
        DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
        ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
        (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
        LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
        ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
        (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
        SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

---




unRAR license restriction
-------------------------

The decompression engine for RAR archives was developed using source
code of unRAR program.  
All copyrights to original unRAR code are owned by Alexander Roshal.

The license for original unRAR code has the following restriction:

    The unRAR sources cannot be used to re-create the RAR compression algorithm,
    which is proprietary. Distribution of modified unRAR sources in separate form
    or as a part of other software is permitted, provided that it is clearly
    stated in the documentation and source comments that the code may
    not be used to develop a RAR (WinRAR) compatible archiver.

--

