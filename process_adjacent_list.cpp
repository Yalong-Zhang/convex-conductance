#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <cwchar>
#endif

namespace {

constexpr std::uint64_t kProgressEvery = 5'000'000ULL;
constexpr std::size_t kIoBufferSize = 1U << 22;

FILE* open_read_binary(const std::filesystem::path& path) {
#if defined(_WIN32)
    return _wfopen(path.wstring().c_str(), L"rb");
#else
    return std::fopen(path.string().c_str(), "rb");
#endif
}

FILE* open_write_binary(const std::filesystem::path& path) {
#if defined(_WIN32)
    return _wfopen(path.wstring().c_str(), L"wb");
#else
    return std::fopen(path.string().c_str(), "wb");
#endif
}

class FastFileReader {
public:
    explicit FastFileReader(const std::filesystem::path& path)
        : file_(open_read_binary(path)), buffer_(kIoBufferSize) {
        if (file_ == nullptr) {
            throw std::runtime_error("failed to open source file");
        }
    }

    ~FastFileReader() {
        if (file_ != nullptr) {
            std::fclose(file_);
        }
    }

    int get() {
        if (pos_ == end_) {
            end_ = std::fread(buffer_.data(), 1, buffer_.size(), file_);
            pos_ = 0;
            if (end_ == 0) {
                return EOF;
            }
        }
        return static_cast<unsigned char>(buffer_[pos_++]);
    }

private:
    FILE* file_ = nullptr;
    std::vector<char> buffer_;
    std::size_t pos_ = 0;
    std::size_t end_ = 0;
};

class FastOutput {
public:
    explicit FastOutput(const std::filesystem::path& path)
        : file_(open_write_binary(path)), buffer_(kIoBufferSize) {
        if (file_ == nullptr) {
            throw std::runtime_error("failed to open output file");
        }
    }

    ~FastOutput() {
        if (file_ != nullptr) {
            try {
                close();
            } catch (...) {
            }
        }
    }

    void write_char(char c) {
        if (pos_ == buffer_.size()) {
            flush();
        }
        buffer_[pos_++] = c;
    }

    void write_uint(std::uint64_t value) {
        char digits[32];
        int n = 0;
        do {
            digits[n++] = static_cast<char>('0' + (value % 10));
            value /= 10;
        } while (value != 0);
        while (n > 0) {
            write_char(digits[--n]);
        }
    }

    void close() {
        if (file_ == nullptr) {
            return;
        }
        flush();
        if (std::fclose(file_) != 0) {
            file_ = nullptr;
            throw std::runtime_error("failed to close output file");
        }
        file_ = nullptr;
    }

private:
    void flush() {
        if (pos_ == 0) {
            return;
        }
        const std::size_t written = std::fwrite(buffer_.data(), 1, pos_, file_);
        if (written != pos_) {
            throw std::runtime_error("failed while writing output file");
        }
        pos_ = 0;
    }

    FILE* file_ = nullptr;
    std::vector<char> buffer_;
    std::size_t pos_ = 0;
};

class DisjointSet {
public:
    std::uint32_t add_node() {
        if (parent_.size() > std::numeric_limits<std::uint32_t>::max()) {
            throw std::runtime_error("too many distinct nodes for uint32 internal ids");
        }
        const auto id = static_cast<std::uint32_t>(parent_.size());
        parent_.push_back(id);
        size_.push_back(1);
        return id;
    }

    std::uint32_t find(std::uint32_t x) {
        while (parent_[x] != x) {
            parent_[x] = parent_[parent_[x]];
            x = parent_[x];
        }
        return x;
    }

    void unite(std::uint32_t a, std::uint32_t b) {
        std::uint32_t ra = find(a);
        std::uint32_t rb = find(b);
        if (ra == rb) {
            return;
        }
        if (size_[ra] < size_[rb]) {
            std::swap(ra, rb);
        }
        parent_[rb] = ra;
        size_[ra] += size_[rb];
    }

    std::uint64_t component_size(std::uint32_t root) const {
        return size_[root];
    }

    std::size_t node_count() const {
        return parent_.size();
    }

private:
    std::vector<std::uint32_t> parent_;
    std::vector<std::uint64_t> size_;
};

class UInt64ToUInt32Map {
public:
    explicit UInt64ToUInt32Map(std::size_t initial_capacity = 1U << 20) {
        rehash(next_power_of_two(initial_capacity));
    }

    std::pair<std::uint32_t, bool> get_or_insert(std::uint64_t key, std::uint32_t new_value) {
        std::size_t slot = find_slot(key);
        if (used_[slot] != 0) {
            return {values_[slot], false};
        }
        if (filled_ + 1 > grow_at_) {
            rehash(keys_.size() * 2);
            slot = find_slot(key);
        }
        used_[slot] = 1;
        keys_[slot] = key;
        values_[slot] = new_value;
        ++filled_;
        return {new_value, true};
    }

    std::size_t size() const {
        return filled_;
    }

private:
    static std::size_t next_power_of_two(std::size_t value) {
        std::size_t result = 16;
        while (result < value) {
            result <<= 1;
        }
        return result;
    }

    static std::uint64_t mix(std::uint64_t x) {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }

    std::size_t find_slot(std::uint64_t key) const {
        const std::size_t mask = keys_.size() - 1;
        std::size_t slot = static_cast<std::size_t>(mix(key)) & mask;
        while (used_[slot] != 0 && keys_[slot] != key) {
            slot = (slot + 1) & mask;
        }
        return slot;
    }

    void rehash(std::size_t new_capacity) {
        new_capacity = next_power_of_two(new_capacity);
        std::vector<std::uint64_t> old_keys;
        std::vector<std::uint32_t> old_values;
        std::vector<std::uint8_t> old_used;
        old_keys.swap(keys_);
        old_values.swap(values_);
        old_used.swap(used_);

        keys_.assign(new_capacity, 0);
        values_.assign(new_capacity, 0);
        used_.assign(new_capacity, 0);
        filled_ = 0;
        grow_at_ = (new_capacity * 7) / 10;

        for (std::size_t i = 0; i < old_used.size(); ++i) {
            if (old_used[i] == 0) {
                continue;
            }
            const std::size_t slot = find_slot(old_keys[i]);
            used_[slot] = 1;
            keys_[slot] = old_keys[i];
            values_[slot] = old_values[i];
            ++filled_;
        }
    }

    std::vector<std::uint64_t> keys_;
    std::vector<std::uint32_t> values_;
    std::vector<std::uint8_t> used_;
    std::size_t filled_ = 0;
    std::size_t grow_at_ = 0;
};

struct ReadStats {
    std::uint64_t parsed_edges = 0;
    std::uint64_t non_self_edges = 0;
    std::uint64_t self_loops = 0;
    std::uint64_t invalid_rows = 0;
};

std::uint64_t make_edge_key(std::uint32_t u, std::uint32_t v) {
    if (u > v) {
        std::swap(u, v);
    }
    return (static_cast<std::uint64_t>(u) << 32) | static_cast<std::uint64_t>(v);
}

std::uint32_t edge_u(std::uint64_t key) {
    return static_cast<std::uint32_t>(key >> 32);
}

std::uint32_t edge_v(std::uint64_t key) {
    return static_cast<std::uint32_t>(key & 0xffffffffULL);
}

void print_read_progress(const ReadStats& stats, const UInt64ToUInt32Map& ids) {
    std::cout << "\rReading graph: " << stats.parsed_edges << " parsed edges, "
              << ids.size() << " nodes, " << stats.self_loops << " self-loops"
              << std::flush;
}

void read_graph(const std::filesystem::path& source_path,
                UInt64ToUInt32Map& ids,
                DisjointSet& dsu,
                std::vector<std::uint64_t>& edges,
                ReadStats& stats) {
    FastFileReader reader(source_path);

    auto get_id = [&](std::uint64_t raw_id) -> std::uint32_t {
        const auto next_id = dsu.node_count();
        if (next_id > std::numeric_limits<std::uint32_t>::max()) {
            throw std::runtime_error("too many distinct nodes for uint32 internal ids");
        }
        auto [id, inserted] = ids.get_or_insert(raw_id, static_cast<std::uint32_t>(next_id));
        if (inserted) {
            dsu.add_node();
        }
        return id;
    };

    auto process_edge = [&](std::uint64_t raw_u, std::uint64_t raw_v) {
        ++stats.parsed_edges;
        const std::uint32_t u = get_id(raw_u);
        const std::uint32_t v = (raw_u == raw_v) ? u : get_id(raw_v);
        if (u == v) {
            ++stats.self_loops;
        } else {
            dsu.unite(u, v);
            edges.push_back(make_edge_key(u, v));
            ++stats.non_self_edges;
        }

        if (stats.parsed_edges % kProgressEvery == 0) {
            print_read_progress(stats, ids);
        }
    };

    std::uint64_t first = 0;
    std::uint64_t second = 0;
    std::uint64_t value = 0;
    int found = 0;
    bool in_number = false;
    bool overflow = false;
    bool line_has_number = false;
    bool processed = false;
    bool ignore_line = false;
    bool bad_line = false;

    auto reset_line = [&]() {
        first = 0;
        second = 0;
        value = 0;
        found = 0;
        in_number = false;
        overflow = false;
        line_has_number = false;
        processed = false;
        ignore_line = false;
        bad_line = false;
    };

    auto finish_number = [&]() {
        line_has_number = true;
        if (overflow) {
            bad_line = true;
            ignore_line = true;
            return;
        }
        if (found == 0) {
            first = value;
            ++found;
        } else if (found == 1) {
            second = value;
            ++found;
            process_edge(first, second);
            processed = true;
        }
        value = 0;
        overflow = false;
    };

    while (true) {
        const int c = reader.get();
        if (c == EOF) {
            if (in_number && !processed && !ignore_line) {
                finish_number();
            }
            if (!processed && (bad_line || (line_has_number && found < 2))) {
                ++stats.invalid_rows;
            }
            break;
        }

        if (!processed && !ignore_line && c >= '0' && c <= '9') {
            const std::uint64_t digit = static_cast<std::uint64_t>(c - '0');
            in_number = true;
            if (!overflow) {
                if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10) {
                    overflow = true;
                } else {
                    value = value * 10 + digit;
                }
            }
            continue;
        }

        if (in_number && !processed && !ignore_line) {
            finish_number();
            in_number = false;
        }

        if (c == '\n') {
            if (!processed && (bad_line || (line_has_number && found < 2))) {
                ++stats.invalid_rows;
            }
            reset_line();
        }
    }

    print_read_progress(stats, ids);
    std::cout << "\nRead graph finished.\n";
}

struct ComponentStats {
    std::uint64_t component_count = 0;
    std::uint32_t largest_root = 0;
    std::uint64_t largest_size = 0;
};

ComponentStats inspect_components(DisjointSet& dsu) {
    ComponentStats stats;
    for (std::uint64_t i = 0; i < dsu.node_count(); ++i) {
        const auto node = static_cast<std::uint32_t>(i);
        const std::uint32_t root = dsu.find(node);
        if (root != node) {
            continue;
        }
        ++stats.component_count;
        const std::uint64_t size = dsu.component_size(root);
        if (size > stats.largest_size) {
            stats.largest_size = size;
            stats.largest_root = root;
        }
    }
    return stats;
}

struct DedupStats {
    std::uint64_t unique_edges = 0;
    std::uint64_t duplicate_edges_removed = 0;
    std::uint64_t max_multiplicity = 0;
};

DedupStats sort_and_deduplicate(std::vector<std::uint64_t>& edges) {
    DedupStats stats;
    if (edges.empty()) {
        return stats;
    }

    std::cout << "Sorting edges...\n";
    std::sort(edges.begin(), edges.end());

    std::cout << "Deduplicating edges...\n";
    std::size_t write = 0;
    std::size_t i = 0;
    while (i < edges.size()) {
        std::size_t j = i + 1;
        while (j < edges.size() && edges[j] == edges[i]) {
            ++j;
        }
        const std::uint64_t multiplicity = static_cast<std::uint64_t>(j - i);
        stats.max_multiplicity = std::max(stats.max_multiplicity, multiplicity);
        edges[write++] = edges[i];
        i = j;
    }

    stats.unique_edges = static_cast<std::uint64_t>(write);
    stats.duplicate_edges_removed = static_cast<std::uint64_t>(edges.size() - write);
    edges.resize(write);
    std::cout << "Deduplication finished.\n";
    return stats;
}

std::vector<std::uint32_t> build_lcc_mapping(DisjointSet& dsu,
                                             const ComponentStats& component_stats,
                                             std::uint64_t& output_max_id) {
    std::vector<std::uint32_t> new_id(dsu.node_count(), 0);
    output_max_id = 0;
    if (component_stats.largest_size == 0) {
        return new_id;
    }
    if (component_stats.largest_size > std::numeric_limits<std::uint32_t>::max()) {
        throw std::runtime_error("largest component is too large for uint32 output ids");
    }

    for (std::uint64_t i = 0; i < dsu.node_count(); ++i) {
        const auto node = static_cast<std::uint32_t>(i);
        if (dsu.find(node) == component_stats.largest_root) {
            new_id[node] = static_cast<std::uint32_t>(++output_max_id);
        }
    }
    return new_id;
}

std::uint64_t count_lcc_edges(const std::vector<std::uint64_t>& edges,
                              const std::vector<std::uint32_t>& new_id) {
    std::uint64_t count = 0;
    for (const std::uint64_t key : edges) {
        if (new_id[edge_u(key)] != 0 && new_id[edge_v(key)] != 0) {
            ++count;
        }
    }
    return count;
}

void write_output(const std::filesystem::path& output_path,
                  const std::vector<std::uint64_t>& edges,
                  const std::vector<std::uint32_t>& new_id,
                  std::uint64_t output_edges,
                  std::uint64_t output_max_id) {
    const auto parent = output_path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    FastOutput output(output_path);
    output.write_uint(output_edges);
    output.write_char(' ');
    output.write_uint(output_max_id);
    output.write_char('\n');

    std::uint64_t written = 0;
    for (const std::uint64_t key : edges) {
        const std::uint32_t u = new_id[edge_u(key)];
        const std::uint32_t v = new_id[edge_v(key)];
        if (u == 0 || v == 0) {
            continue;
        }
        output.write_uint(u);
        output.write_char(' ');
        output.write_uint(v);
        output.write_char('\n');
        ++written;
        if (written % kProgressEvery == 0) {
            std::cout << "\rWriting output: " << written << " / " << output_edges
                      << " edges" << std::flush;
        }
    }
    output.close();
    std::cout << "\rWriting output: " << written << " / " << output_edges
              << " edges\nWriting output finished.\n";
}

}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 3) {
            std::cerr << "Usage: " << argv[0] << " <source_edge_list> <output_file>\n";
            return 1;
        }

        const std::filesystem::path source_path = argv[1];
        const std::filesystem::path output_path = argv[2];

        UInt64ToUInt32Map ids;
        DisjointSet dsu;
        std::vector<std::uint64_t> edges;
        ReadStats read_stats;

        read_graph(source_path, ids, dsu, edges, read_stats);

        const ComponentStats component_stats = inspect_components(dsu);
        std::cout << "Extract largest connected component finished.\n";

        const DedupStats dedup_stats = sort_and_deduplicate(edges);

        std::uint64_t output_max_id = 0;
        std::vector<std::uint32_t> new_id = build_lcc_mapping(dsu, component_stats, output_max_id);
        const std::uint64_t output_edges = count_lcc_edges(edges, new_id);
        std::cout << "Prepare output mapping finished.\n";

        write_output(output_path, edges, new_id, output_edges, output_max_id);

        std::cout << "Summary:\n"
                  << "  Output edges: " << output_edges << '\n'
                  << "  Output max node id: " << output_max_id << '\n'
                  << "  Source connected components: " << component_stats.component_count << '\n'
                  << "  Maximum source edge multiplicity: " << dedup_stats.max_multiplicity << '\n'
                  << "  Removed self-loops: " << read_stats.self_loops << '\n'
                  << "  Parsed source edges: " << read_stats.parsed_edges << '\n'
                  << "  Source nodes: " << dsu.node_count() << '\n'
                  << "  Removed duplicate edge records: " << dedup_stats.duplicate_edges_removed << '\n';
        if (read_stats.invalid_rows != 0) {
            std::cout << "  Skipped invalid rows: " << read_stats.invalid_rows << '\n';
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
