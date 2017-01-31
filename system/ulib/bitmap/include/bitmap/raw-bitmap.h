// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <bitmap/bitmap.h>

#include <stddef.h>
#include <stdint.h>

#include <magenta/types.h>
#include <mxtl/macros.h>
#include <mxtl/unique_ptr.h>

namespace bitmap {

// A simple bitmap backed by an array.
class RawBitmap final : public Bitmap {
public:
    RawBitmap();
    virtual ~RawBitmap() = default;
    RawBitmap(RawBitmap&& rhs) = default;
    RawBitmap& operator=(RawBitmap&& rhs) = default;
    DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(RawBitmap);

    // Returns the size of this bitmap.
    size_t size(void) const { return size_; }

    // Resets the bitmap; clearing and resizing it.
    // Allocates memory, and can fail.
    mx_status_t Reset(size_t size);

    // Shrinks the accessible portion of the bitmap, without re-allocating
    // the underlying storage.
    //
    // This is useful for programs which require underlying bitmap storage
    // to be aligned to a certain size (initialized via Reset), but want to
    // restrict access to a smaller portion of the bitmap (via Shrink).
    mx_status_t Shrink(size_t size);

    // Returns the lesser of bitmax and the index of the first bit that doesn't
    // match *is_set* starting from *bitoff*.
    size_t Scan(size_t bitoff, size_t bitmax, bool is_set) const;

    // Find a run of *run_len* *set* bits, between bitoff and bitmax.
    // Returns the start of the run in *bitoff_start*, or bitmax if it is
    // not found in the provided range.
    // If the run is not found, "ERR_NO_RESOURCES" is returned.
    mx_status_t Find(bool set, size_t bitoff, size_t bitmax, size_t run_len,
                     size_t* bitoff_start) const;

    // Returns true if all the bits in [*bitoff*, *bitmax*) are set. Afterwards,
    // *first_unset* will be set to the lesser of bitmax and the index of the
    // first unset bit after *bitoff*.
    bool Get(size_t bitoff, size_t bitmax,
             size_t* first_unset = nullptr) const override;

    // Sets all bits in the range [*bitoff*, *bitmax*).  Returns an error if
    // bitmax < bitoff or size_ < bitmax, and NO_ERROR otherwise.
    mx_status_t Set(size_t bitoff, size_t bitmax) override;

    // Clears all bits in the range [*bitoff*, *bitmax*).  Returns an error if
    // bitmax < bitoff or size_ < bitmax, and NO_ERROR otherwise.
    mx_status_t Clear(size_t bitoff, size_t bitmax) override;

    // Clear all bits in the bitmap.
    void ClearAll() override;

    // This function allows access to underlying data, but is dangerous: It
    // leaks the pointer to bits_. Reset and the bitmap destructor should not
    // be called on the bitmap while the pointer returned from data() is alive.
    const void* data_unsafe() const { return bits_.get(); }

private:
    // The size of this bitmap, in bits.
    size_t size_;

    // The array backing this bitmap.
    mxtl::unique_ptr<size_t[]> bits_;
};

} // namespace bitmap
