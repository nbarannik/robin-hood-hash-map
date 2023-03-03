#pragma once

#include <functional>
#include <vector>
#include <optional>
#include <initializer_list>
#include <stdexcept>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
    using SizeType = size_t;
    class Node;

public:
    static constexpr double LOAD_FACTOR = 0.9;

    class iterator {
    public:
        using TableIterator = typename std::vector<HashMap::Node>::iterator;

        iterator() = default;

        std::pair<const KeyType, ValueType>* get_pointer() const {
            return reinterpret_cast<std::pair<const KeyType, ValueType>*>(&((*table_iter_).value));
        }

        std::pair<const KeyType, ValueType>& operator*() const {
            return *get_pointer();
        }

        std::pair<const KeyType, ValueType>* operator->() const {
            return reinterpret_cast<std::pair<const KeyType, ValueType>*>(&(*table_iter_).value);
        }

        HashMap::iterator& operator++() {
            find_next_element();
            return *this;
        }

        HashMap::iterator operator++(int) {
            HashMap::iterator previous = *this;
            find_next_element();
            return previous;
        }

        bool operator==(HashMap::iterator other) {
            return (table_iter_ == other.table_iter_);
        }

        bool operator!=(HashMap::iterator other) {
            return !(*this == other);
        }

        friend HashMap;
    private:
        void find_next_element() {
            ++table_iter_;
            while (table_iter_ != table_end_ && (*table_iter_).empty) {
                ++table_iter_;
            }
        }

        iterator(TableIterator it, TableIterator end) : table_iter_(it), table_end_(end) {}

        TableIterator table_iter_;
        TableIterator table_end_;
    };

    friend HashMap::iterator;

    class const_iterator {
    public:
        const_iterator() = default;

        using ConstTableIterator = typename std::vector<HashMap::Node>::const_iterator;

        const std::pair<const KeyType, ValueType>* get_pointer() const {
            return reinterpret_cast<const std::pair<const KeyType, ValueType>*>(&((*table_iter_).value));
        }

        const std::pair<const KeyType, ValueType>& operator*() const {
            return *get_pointer();
        }

        const std::pair<const KeyType, ValueType>* operator->() const {
            return reinterpret_cast<const std::pair<const KeyType, ValueType>*>(&(*table_iter_).value);
        }

        HashMap::const_iterator& operator++() {
            find_next_element();
            return *this;
        }

        HashMap::const_iterator operator++(int) {
            HashMap::const_iterator previous = *this;
            find_next_element();
            return previous;
        }

        bool operator==(HashMap::const_iterator other) {
            return (table_iter_ == other.table_iter_);
        }

        bool operator!=(HashMap::const_iterator other) {
            return !(*this == other);
        }

        friend HashMap;
    private:
        void find_next_element() {
            ++table_iter_;
            while (table_iter_ != table_end_ && (*table_iter_).empty) {
                ++table_iter_;
            }
        }

        const_iterator(ConstTableIterator it, ConstTableIterator end) : table_iter_(it), table_end_(end) {}
        ConstTableIterator table_iter_;
        ConstTableIterator table_end_;
    };

    friend HashMap::const_iterator;

    explicit HashMap(const Hash& hasher = Hash()) : hasher_(hasher) {}

    template<class IteratorType>
    HashMap(IteratorType begin, IteratorType end, const Hash& hasher = Hash()) : hasher_(hasher) {
        while (begin != end) {
            insert(*begin);
            ++begin;
        }
    }

    HashMap(std::initializer_list<std::pair<KeyType, ValueType>> list, const Hash& hasher = Hash()) : hasher_(hasher) {
        for (auto& i : list) {
            insert(i);
        }
    }

    bool empty() const {
        return (size_ == 0);
    }

    SizeType size() const {
        return size_;
    }

    Hash hash_function() const {
        return hasher_;
    }

    ValueType& insert(std::pair<KeyType, ValueType> el) {
        if (static_cast<double>(capacity_) * LOAD_FACTOR <= static_cast<double>(size_)) {
            rehash();
        }

        SizeType pos = get_position(el.first);
        SizeType probe_length = 0;
        std::optional<SizeType> insert_index = std::nullopt;

        for (SizeType i = 0; i < capacity_; ++i) {
            if (table_[pos].empty) {
                if (!insert_index) {
                    insert_index = pos;
                }

                table_[pos].empty = false;
                table_[pos].value = el;
                table_[pos].probe_seq_length = probe_length;
                ++size_;
                return table_[*insert_index].value.second;
            }
            else if (table_[pos].probe_seq_length < probe_length) {
                if (!insert_index) {
                    insert_index = pos;
                }

                std::swap(table_[pos].value, el);
                std::swap(table_[pos].probe_seq_length, probe_length);
            }
            else if (table_[pos].value.first == el.first) {
                if (!insert_index) {
                    insert_index = pos;
                }
                return table_[*insert_index].value.second;
            }

            ++pos;
            ++probe_length;

            if (pos == capacity_) {
                pos = 0;
            }
        }

        throw std::logic_error("wrong algorithm");
    }

    void erase(const KeyType& key) {
        std::optional<SizeType> index = find_element_index(key);
        if (index) {
            SizeType pos = *index;
            table_[pos].empty = true;
            --size_;

            while (!table_[get_next(pos)].empty && table_[get_next(pos)].probe_seq_length) {
                table_[get_next(pos)].probe_seq_length--;
                std::swap(table_[pos], table_[get_next(pos)]);
                pos = get_next(pos);
            }
        }
    }

    HashMap::iterator find(const KeyType& key) {
        std::optional<SizeType> index = find_element_index(key);
        if (index) {
            auto table_it = table_.begin() + *index;
            return iterator(table_it, table_.end());
        }
        else {
            return iterator(table_.end(), table_.end());
        }
    }

    HashMap::const_iterator find(const KeyType& key) const {
        std::optional<SizeType> index = find_element_index(key);
        if (index) {
            auto table_it = table_.begin() + *index;
            return const_iterator(table_it, table_.end());
        }
        else {
            return const_iterator(table_.end(), table_.end());
        }
    }

    ValueType& operator[](const KeyType& key) {
        std::optional<SizeType> index = (size_ == 0 ? std::nullopt : find_element_index(key));

        if (index) {
            return table_[*index].value.second;
        }

        std::pair<KeyType, ValueType> insert_pair = { key, ValueType() };
        return insert(insert_pair);
    }

    const ValueType& at(const KeyType& key) const {
        std::optional<SizeType> index = (size_ == 0 ? std::nullopt : find_element_index(key));

        if (index) {
            return table_[*index].value.second;
        }

        throw std::out_of_range("There is no element this key.");
    }

    void clear() {
        size_ = 0;
        capacity_ = 0;
        table_.clear();
    }

    HashMap::iterator begin() {
        if (size_ == 0) {
            return end();
        }

        HashMap::iterator res(table_.begin(), table_.end());

        if ((*res.table_iter_).empty) {
            res.find_next_element();
        }
        return res;
    }

    HashMap::iterator end() {
        return HashMap::iterator(table_.end(), table_.end());
    }

    HashMap::const_iterator begin() const {
        HashMap::const_iterator res(table_.begin(), table_.end());
        if ((*res.table_iter_).empty) {
            res.find_next_element();
        }
        return res;
    }

    HashMap::const_iterator end() const {
        return HashMap::const_iterator(table_.end(), table_.end());
    }

private:
    inline SizeType get_position(const KeyType& key) const {
        return hasher_(key) % capacity_;
    }

    inline SizeType get_next(SizeType index) const {
        return (index + 1) % capacity_;
    }

    std::optional<SizeType> find_element_index(const KeyType& key) const {
        if (size_ == 0) {
            return std::nullopt;
        }

        SizeType pos = get_position(key);
        SizeType probe_length = 0;

        for (SizeType i = 0; i < capacity_; ++i) {
            if (table_[pos].empty or table_[pos].probe_seq_length < probe_length) {
                return std::nullopt;
            }
            else if (table_[pos].value.first == key) {
                return pos;
            }

            ++pos;
            ++probe_length;

            if (pos == capacity_) {
                pos = 0;
            }
        }

        return std::nullopt;
    }

    SizeType get_next_simple(SizeType min_capacity_) {
        while (simple_capacity_id < CAPACITIES_SIZE && SIMPLE_CAPACITIES[simple_capacity_id] < min_capacity_) {
            ++simple_capacity_id;
        }

        if (SIMPLE_CAPACITIES[simple_capacity_id] < min_capacity_) {
            throw std::out_of_range("Capacity error.");
        }

        return SIMPLE_CAPACITIES[simple_capacity_id];
    }

    void resize() {
        capacity_ = get_next_simple(((capacity_ + 1) << 1));
        table_.assign(capacity_, Node{});
    }

    void rehash() {
        std::vector<Node> prev_table = table_;
        resize();
        size_ = 0;

        for (auto& node : prev_table) {
            if (!node.empty) {
                insert(node.value);
            }
        }
    }

    class Node {
    public:
        Node(std::pair<KeyType, ValueType> value, SizeType probe_seq_length) : value(value), empty(false), probe_seq_length(probe_seq_length) {}
        Node() = default;

        std::pair<KeyType, ValueType> value;
        SizeType probe_seq_length = 0;
        bool empty = true;
    };

    SizeType size_ = 0;
    SizeType capacity_ = 0;
    std::vector<Node> table_;
    Hash hasher_;
    SizeType simple_capacity_id = 0;

    SizeType CAPACITIES_SIZE = 72;
    static constexpr SizeType SIMPLE_CAPACITIES[] = {
2, 3, 5, 7, 11, 17, 23, 31, 41, 59, 79, 103, 137, 179, 233, 307, 401, 523, 683, 907, 1181, 1543, 2011, 2617, 3407, 4441, 5779, 7517, 9781, 12721, 16547, 21517, 27983, 36383, 47303, 61507, 79967, 103963, 135173, 175727, 228451, 296987, 386093, 501931, 652541, 848321, 1102823, 1433681, 1863787, 2422939, 3149821, 4094791, 5323229, 6920201, 8996303, 11695231, 15203803, 19764947, 25694447, 33402793, 43423631, 56450731, 73385953, 95401759, 124022287, 161228983, 209597693, 272477017, 354220127, 460486217, 598632137, 778221781 };
};