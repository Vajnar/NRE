/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <Exception.h>

namespace nre {

class SyscallException : public Exception {
public:
    explicit SyscallException(ErrorCode code = E_FAILURE, const String &msg = String()) throw()
        : Exception(code, msg) {
    }

    virtual void write(OStream& os) const;
};

}

#ifdef __i386__
#    include <arch/x86_32/SyscallABI.h>
#elif defined __x86_64__
#    include <arch/x86_64/SyscallABI.h>
#else
#    error "Unsupported architecture"
#endif
