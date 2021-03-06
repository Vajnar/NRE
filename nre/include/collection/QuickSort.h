/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2010, Julian Stecklina <jsteckli@os.inf.tu-dresden.de>
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

#include <util/Util.h>

namespace nre {

/**
 * Quicksort implementation
 */
template<typename T>
class Quicksort {
public:
    typedef bool (*cmp_func)(T const &a, T const &b);

    /**
     * Sorts the given array with the given comparison function
     *
     * @param cmp the comparison function
     * @param a the array
     * @param count the number of items in the array
     */
    static void sort(cmp_func cmp, T a[], size_t count) {
        if(count > 1)
            sort(cmp, a, 0, count - 1);
    }

private:
    static void sort(cmp_func cmp, T a[], ssize_t left, ssize_t right) {
        if(right > left) {
            ssize_t pivot_i = partition(cmp, a, left, right, (left + right) / 2);
            sort(cmp, a, left, pivot_i - 1);
            sort(cmp, a, pivot_i + 1, right);
        }
    }

    static int partition(cmp_func cmp, T a[], ssize_t left, ssize_t right, ssize_t pivot_i) {
        Util::swap<T>(a[pivot_i], a[right]);
        T &pivot = a[right];
        ssize_t si = left;

        for(ssize_t i = left; i < right; i++) {
            if(cmp(a[i], pivot))
                Util::swap<T>(a[i], a[si++]);
        }

        Util::swap<T>(a[si], a[right]);
        return si;
    }

    Quicksort();
};

}
