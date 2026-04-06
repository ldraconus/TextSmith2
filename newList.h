#pragma once

#include <QList>

#include <vector>

template <typename T>
class List {
private:
    std::vector<T> mData;

    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

public:
    List() = default;
    List(const List<T>& m)     { mData = m.mData; }
    List(List<T>&& m) noexcept { mData = m.mData; }
    List(const qlonglong pre)  { mData.resize(4); }

    List(const QList<T>& list) {
        for (auto i = 0; i < list.size(); ++i) mData.push_back(list[i]);
    }
    List(std::initializer_list<T>& m) {
        for (const auto& v: m) mData.push_back(v);
    }
    List(const std::initializer_list<T>& m) {
        for (const auto& v: m) mData.push_back(v);
    }

    auto begin()         { return mData.begin(); }
    auto end()           { return mData.end(); }
    auto rbegin()        { return mData.rbegin(); }
    auto rend()          { return mData.rend(); }
    auto begin() const   { return mData.begin(); }
    auto end() const     { return mData.end(); }
    auto cbegin() const  { return mData.cbegin(); }
    auto cend() const    { return mData.cend(); }
    auto rcbegin() const { return mData.rcbegin(); }
    auto rcend() const   { return mData.rcend(); }

    List& operator=(const List<T>& m)     { mData = m.mData; return *this; }
    List& operator=(List<T>&& m) noexcept { mData = m; m.mData.clear(); return *this; }

    List& operator=(const std::initializer_list<const T>& m) {
        mData.clear;
        for (const auto& v: m) mData.append(v);
        return *this;
    }
    List& operator=(std::initializer_list<const T>&& m) {
        mData.clear;
        for (const auto& v: m) mData.append(v);
        return *this;
    }

    T&       operator[](qlonglong idx)               { return mData[idx]; }
    const T& operator[](qlonglong idx) const         { return mData[idx]; }
    const T& at(qlonglong idx) const                 { if (idx >= 0 && idx < size()) return mData[idx]; else throw("Index out of range"); }
    List<T>& operator<<(const T& v)                  { append(v); return *this; }
    List<T>& operator<<(std::initializer_list<T>& v) { for (const auto& x: v) append(x); }

    void append(const T& v)                                      { mData.push_back(v); }
    void append(std::initializer_list <T>& m)                    { for (const auto& x: m) mData.push_back(x); }
    void push_back(const T& v)                                   { append(v); }
    void insert(const_iterator it, const T& v)                   { mData.insert(it, v); }
    void insert(qlonglong idx, const T& v)                       { insert(iteratorOf(idx), v); }
    void insert(qlonglong& idx, const T& v)                      { insert(iteratorOf(idx), v); }
    void insert(const_iterator it, std::initializer_list <T>& m) { for (const auto& x: m) mData.insert(it, x); }
    void insert(qlonglong idx, std::initializer_list <T>& m)     { insert(iteratorOf(idx), m); }
    void insert(qlonglong& idx, std::initializer_list <T>& m)    { insert(iteratorOf(idx), m); }

    void sort() { return mData.sort(); }

    QList<T> toQList() { QList<T> lst; for (const auto& itm: mData) lst.append(itm); return lst; }

    T& first()    { return mData[0]; }
    T& last()     { return mData[mData.size() - 1]; }
    T takeFirst() { T f = first(); remove(qlonglong(0)); return f; }
    T takeLast()  { T l = last(); remove(mData.size() - 1); }

    void clear()                         { mData.clear(); }
    void erase(const T& v)               { if (contains(v)) mData.erase(find(v)); }
    void erase(const const_iterator& it) { mData.erase(it); }
    void erase(const iterator &it)       { mData.erase(it); }
    void remove(const qlonglong idx)     { erase(iteratorOf(idx)); }
    void remove(const const_iterator& i) { erase(i); }
    void remove(const T& v)              { if (contains(v)) mData.erase(find(v)); }

    bool contains(T& v) const       { return find(v) != end(); }
    bool contains(const T& v) const { return find(v) != end(); }
    auto find(T& v) const           { for (auto it = cbegin(); it != cend(); ++it) if (*it == v) return it; return cend(); }
    auto find(const T& v) const     { for (auto it = cbegin(); it != cend(); ++it) if (*it == v) return it; return cend(); }
    auto indexOf(const T& v) const  { for (auto i = 0; i < count(); ++i) { if (mData[i] == v) return i; } return -1; }

    auto iteratorOf(qlonglong idx) const {
        auto it = mData.cbegin();
        for (auto i = 0; i != idx && it != mData.cend(); ++i, ++it) {}
        return it;
    }

    auto iteratorOf(qlonglong idx) {
        auto it = mData.begin();
        for (auto i = 0; i != idx && it != mData.end(); ++i, ++it) {}
        return it;
    }

    auto swapItemsAt(const qlonglong idx1, qlonglong idx2) { std::swap<T>(mData[idx1], mData[idx2]); }
    T    takeAt(qlonglong idx)                             { T r = mData[idx]; remove(idx); return r; }

    auto      size() const    { return qlonglong(mData.size()); }
    bool      empty() const   { return mData.empty(); }
    bool      isEmpty() const { return empty(); }
    qlonglong count() const   { return size(); }
};
