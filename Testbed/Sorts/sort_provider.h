#pragma once
#include <ACL/general.h>
#include <ACL/vector.h>
#include <ACL/gui/color.h>
#include <algorithm>


using abel::gui::Color;


template <typename T>
class SortProvider {
public:
    using elem_t = T;


    virtual void sort(T *data, size_t size) = 0;

    virtual Color getColor() const = 0;

    virtual const char *getName() const = 0;

};


namespace sort_providers {


template <typename T>
class StdSortProvider : public SortProvider<T> {
public:
    StdSortProvider() = default;

    virtual void sort(T *data, size_t size) override {
        assert(data);

        std::sort(data, data + size);
    }

    virtual Color getColor() const override {
        return Color::RED * 0.8f;
    }

    virtual const char *getName() const override {
        return "std::sort";
    }
};

template <typename T>
class StdStableSortProvider : public SortProvider<T> {
public:
    StdStableSortProvider() = default;

    virtual void sort(T *data, size_t size) override {
        assert(data);

        std::stable_sort(data, data + size);
    }

    virtual Color getColor() const override {
        return Color::ORANGE * 0.8f;
    }

    virtual const char *getName() const override {
        return "std::stable_sort";
    }
};

template <typename T>
class InplaceMergeSortProvider : public SortProvider<T> {
public:
    InplaceMergeSortProvider() = default;

    virtual void sort(T *data, size_t size) override {
        assert(data);

        if (size <= 1) {
            return;
        }

        T *start = data;
        T *middle = data + size / 2;
        T *end = data + size;

        sort(start, middle - start);
        sort(middle, end - middle);
        std::inplace_merge(start, middle, end);
    }

    virtual Color getColor() const override {
        return Color::YELLOW * 0.8f;
    }

    virtual const char *getName() const override {
        return "inplace merge sort";
    }
};

template <typename T>
class BubbleSortProvider : public SortProvider<T> {
public:
    BubbleSortProvider() = default;

    virtual void sort(T *data, size_t size) override {
        assert(data);

        bool swapped = false;

        do {
            swapped = false;

            for (size_t i = 1; i < size; ++i) {
                if (data[i - 1] > data[i]) {
                    std::swap(data[i - 1], data[i]);
                    swapped = true;
                }
            }

            --size;
        } while (swapped);
    }

    virtual Color getColor() const override {
        return Color::BLUE * 0.8f;
    }

    virtual const char *getName() const override {
        return "bubble sort";
    }
};

template <typename T>
class InsertionSortProvider : public SortProvider<T> {
public:
    InsertionSortProvider() = default;

    virtual void sort(T *data, size_t size) override {
        assert(data);

        for (unsigned i = 1; i < size; ++i) {
            T cur = std::move(data[i]);

            unsigned target = i;
            while (target > 0 && data[target - 1] > cur) {
                data[target] = std::move(data[target - 1]);
                --target;
            }

            data[target] = std::move(cur);
        }
    }

    virtual Color getColor() const override {
        return Color::GREEN * 0.8f;
    }

    virtual const char *getName() const override {
        return "insertion sort";
    }
};


}
