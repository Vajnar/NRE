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

#include <arch/Types.h>

namespace nre {

class OStream;

/**
 * An object of CapRange represents a range of capabilities that can be passed around (not in the
 * sense that they are delegated!). This way, you can e.g. specify when using the Utcb what
 * capabilies you want to delegate and the Utcb figures out how to actually do that by making
 * sure that all typed items are naturally aligned.
 */
class CapRange {
public:
    static const uintptr_t NO_HOTSPOT   = -1;

    /**
     * Constructor. Creates an empty range
     */
    explicit CapRange() : _start(), _count(), _attr(), _hotspot() {
    }
    /**
     * Constructor
     *
     * @param start the start of the range
     * @param count the length of the range (not the order, as required by NOVA)
     * @param attr the attributes
     * @param hotspot the hotspot (default: no hotspot)
     */
    explicit CapRange(uintptr_t start, size_t count, uint attr, uintptr_t hotspot = NO_HOTSPOT)
        : _start(start), _count(count), _attr(attr), _hotspot(hotspot) {
    }

    /**
     * Revokes this range of capabilities
     *
     * @param self whether to revoke it from yourself as well
     */
    void revoke(bool self);

    /**
     * Limits count() to a value that ensures that at most <free_typed> typed items are necessary
     * in the UTCB to delegate this range.
     *
     * @param free_typed the number of free typed items you have
     */
    void limit_to(size_t free_typed);

    /**
     * The start of this range
     */
    uintptr_t start() const {
        return _start;
    }
    void start(uintptr_t start) {
        _start = start;
    }
    /**
     * The length of this range
     */
    size_t count() const {
        return _count;
    }
    void count(size_t count) {
        _count = count;
    }
    /**
     * The attributes
     */
    uint attr() const {
        return _attr;
    }
    void attr(uint attr) {
        _attr = attr;
    }
    /**
     * The hotspot
     */
    uintptr_t hotspot() const {
        return _hotspot;
    }
    void hotspot(uintptr_t hotspot) {
        _hotspot = hotspot;
    }

private:
    uintptr_t _start;
    size_t _count;
    uint _attr;
    uintptr_t _hotspot;
};

OStream &operator<<(OStream &os, const CapRange &cr);

}
