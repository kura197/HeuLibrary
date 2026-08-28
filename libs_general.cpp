
void read_args(int argc, char* argv[]) {
    [[maybe_unused]] auto read_float_arg = [&argc, &argv](double& param, int idx) {
        if (argc >= idx+1) {
            param = atof(argv[idx]);
            DEBUG("load {} arg: {:.5f}\n", idx, param);
        } else {
            DEBUG("failed to read {} arg\n", idx);
        }
    };

    [[maybe_unused]] auto read_int_arg = [&argc, &argv](int& param, int idx) {
        if (argc >= idx+1) {
            param = atoi(argv[idx]);
            DEBUG("load {} arg: {}\n", idx, param);
        } else {
            DEBUG("failed to read {} arg\n", idx);
        }
    };

    // example
    //if (argc >= 2) Env::start_temp = atof(argv[1]);
    //if (argc >= 3) Env::end_temp = atof(argv[2]);
    // TODO: test
    //read_float_arg(Env::start_temp, 1);
    //read_float_arg(Env::end_temp, 1);
}

////////////////////////////////////////////////////////////////////
//# replace unordered_set

// random access: O(1)
// insert: O(1)
// erase: O(1)
// check: O(1)
// max value: fixed
template <int N>
class IndexSet {
private:
    int data[N];
    int indices[N];
    int num = 0;

public:
    using iterator = int*;
    using const_iterator = const int*;

    IndexSet() {
        memset(indices, -1, sizeof(indices));
    }

    iterator begin() {
        return data;
    }

    iterator end() {
        return data + num;
    }

    const_iterator begin() const {
        return data;
    }

    const_iterator end() const {
        return data + num;
    }

    bool insert(int a) {
        assert(a >= 0 && a < N);
        if (indices[a] != -1)
            return false;
        data[num] = a;
        indices[a] = num;
        num++;
        return true;
    }

    bool erase(int a) {
        assert(a >= 0 && a < N);
        int index = indices[a];
        if (index == -1)
            return false;
        assert(num > 0);
        data[index] = data[--num];
        indices[data[index]] = index;
        indices[a] = -1;
        return true;
    }

    void clear() {
        memset(indices, -1, sizeof(indices));
        num = 0;
    }

    bool contains(int a) const {
        return indices[a] != -1;
    }

    const int& operator[](int i) const {
        assert(i >= 0 && i < num);
        return data[i];
    }

    int size() const {
        return num;
    }

    bool empty() const {
        return num == 0;
    }
};

//////////////////////////////////////////////////
//# replace unordered_map

// 連想配列
// Keyにハッシュ関数を適用しない
// open addressing with linear probing
// unordered_mapよりも速い
// nは格納する要素数よりも4~16倍ほど大きくする
template <class Key, class T>
struct HashMap {
    public:
        explicit HashMap(uint32_t n) {
            n_ = n;
            valid_.resize(n_, false);
            data_.resize(n_);
        }

        // 戻り値
        // - 存在するならtrue、存在しないならfalse
        // - index
        pair<bool,int> get_index(Key key) const {
            Key i = key % n_;
            while (valid_[i]) {
                if (data_[i].first == key) {
                    return {true, i};
                }
                if (++i == n_) {
                    i = 0;
                }
            }
            return {false, i};
        }

        // 指定したindexにkeyとvalueを格納する
        void set(int i, Key key, T value) {
            valid_[i] = true;
            data_[i] = {key, value};
        }

        // 指定したindexのvalueを返す
        T get(int i) const {
            assert(valid_[i]);
            return data_[i].second;
        }

        void clear() {
            fill(valid_.begin(), valid_.end(), false);
        }

    private:
        uint32_t n_;
        vector<bool> valid_;
        vector<pair<Key,T>> data_;
};


//////////////////////////////////////////////////

// あるマスを削除した際に残りが連結か判定するテーブルを作成. (https://atcoder.jp/contests/ahc039/submissions/59648665)
// 消去不可と判定された場合でも、3x3領域外を辿って連結となる場合がある
/// 0 1 2
/// 3 . 5
/// 6 7 8
// Usage:
//   auto con9 = connect9();
//   bool can_delete = con9[get_mask9(field, y, x)];
vector<bool> connect9() {
    vector<bool> ok(1 << 9, false);
    for (int mask = 0; mask < (1 << 9); ++mask) {
        int k = 0;
        for (int i = 0; i < 4; ++i) {
            if ((mask >> (i * 2 + 1)) & 1) {
                ++k;
            }
        }
        vector<tuple<int, int, int>> triplets = {
            {0, 1, 3},
            {1, 2, 5},
            {3, 6, 7},
            {5, 7, 8}
        };
        for (auto [a, b, c] : triplets) {
            if ((mask >> a & 1) && (mask >> b & 1) && (mask >> c & 1)) {
                --k;
            }
        }
        ok[mask] = (k == 1);
    }
    return ok;
}

// 3x3の周囲マスを走査して、ビットマスクを取得. field[y][x] := (y, x)マスが通行可能
int get_mask9(const vector<vector<bool>>& field, int i, int j) {
    int k = 0;
    int mask = 0;
    for (int di = -1; di <= 1; ++di) {
        int i2 = i + di;
        for (int dj = -1; dj <= 1; ++dj) {
            int j2 = j + dj;
            if (i2 >= 0 && i2 < (int)field.size() && j2 >= 0 && j2 < (int)field[i2].size() && field[i2][j2]) {
                mask |= (1 << k);
            }
            ++k;
        }
    }
    return mask;
}

///////////////////////////////////////////////////////////////////

struct RandomPermutation {
    int cursor = 0;
    int n;
    vector<int> perm;

    RandomPermutation(int in_n) : n(in_n), perm(in_n) {
        iota(ALL(perm), 0);
    }

    void reset_cursor() {
        cursor = 0;
    }

    int next() {
        swap(perm[cursor], perm[rand_int(cursor, n)]);
        return perm[cursor++];
    }
};

////////////////////////////////////////////////////////////////////

template <typename T>
class VectorQueue {
private:
    std::vector<T> data_; // データを格納するvector
    size_t head_;         // キューの先頭を指すインデックス

public:
    /**
     * @brief コンストラクタ
     * @param max_elements 予想される最大の要素数。このサイズでメモリを事前確保します。
     */
    explicit VectorQueue(size_t max_elements) : head_(0) {
        data_.reserve(max_elements);
    }

    /**
     * @brief キューの末尾に要素を追加します。
     * @param value 追加する要素
     */
    void push(const T& value) {
        data_.push_back(value);
    }

    /**
     * @brief キューの末尾に要素を直接構築して追加します (ムーブ、完全転送に対応)。
     * @tparam Args コンストラクタ引数の型
     * @param args 要素を構築するための引数
     */
    template <typename... Args>
    void emplace(Args&&... args) {
        data_.emplace_back(std::forward<Args>(args)...);
    }

    /**
     * @brief キューの先頭から要素を削除します (実際にはインデックスを移動)。
     */
    void pop() {
        if (empty()) {
            // 空のキューに対してpopを呼ぶのは未定義動作だが、安全のため例外を投げることもできる
            // throw std::out_of_range("pop() called on empty queue");
            return;
        }
        head_++;
    }

    /**
     * @brief キューの先頭要素への参照を返します。
     * @return 先頭要素への参照
     */
    T& front() {
        return data_[head_];
    }

    /**
     * @brief キューの先頭要素へのconst参照を返します。
     * @return 先頭要素へのconst参照
     */
    const T& front() const {
        return data_[head_];
    }

    /**
     * @brief キューが空かどうかを判定します。
     * @return 空ならtrue, そうでなければfalse
     */
    bool empty() const noexcept {
        return head_ >= data_.size();
    }

    /**
     * @brief キューに含まれる現在の要素数を返します。
     * @return 要素数
     */
    size_t size() const noexcept {
        return data_.size() - head_;
    }

    /**
     * @brief キューをクリアし、再利用可能な状態にします。
     * メモリは解放せず、保持したままになります。
     */
    void clear() {
        data_.clear();
        head_ = 0;
    }

    /**
     * @brief 内部vectorのキャパシティ（事前確保したメモリサイズ）を返します。
     * @return キャパシティ
     */
    size_t capacity() const noexcept {
        return data_.capacity();
    }
};

///////////////////////////////////////////////////////////////////

// move_to[pos][dir] := 座標posから方向dに移動したときの遷移先。壁があると同じ場所にとどまる
// can_move[pos][dir] := posからd方向に壁が無く移動可能かどうか
array<array<int, 4>, H*W> move_to;
array<array<bool, 4>, H*W> can_move;
void build_move_vec() {
    REP(h, H) REP(w, W) {
        REP(d, 4) move_to[h*H + w][d] = h*H + w;
        if (w < W-1 && !input::h[h][w]) move_to[h*H + w][0] = h*H + (w+1);
        if (h < H-1 && !input::v[h][w]) move_to[h*H + w][1] = (h+1)*H + w;
        if (w > 0 && !input::h[h][w-1]) move_to[h*H + w][2] = h*H + (w-1);
        if (h > 0 && !input::v[h-1][w]) move_to[h*H + w][3] = (h-1)*H + w;
    }

    REP(i, H*W) {
        REP(d, 4) {
            if (move_to[i][d] == i) can_move[i][d] = false;
            else can_move[i][d] = true;
        }
    }
}

////////////////////////////////////////////////////////////////////

// TODO: flatten
struct Pos {
    using T = int;
    T y, x;
    Pos() : y(0), x(0) {};
    Pos(T _y, T _x) : y(_y), x(_x) {};

    auto operator<=>(const Pos&) const = default;

    Pos operator+(const Pos& rhs) {
        return Pos(x + rhs.x, y + rhs.y);
    }

    Pos operator-(const Pos& rhs) {
        return Pos(x - rhs.x, y - rhs.y);
    }

    ull get_hash() const {
        // TODO: check collision
        return (x + 1) * 1000000007ULL + (y + 1);
    }
};

std::ostream& operator<<(std::ostream& os, const Pos& pos) {
    os << "(" << pos.y << ", " << pos.x << ")";
    return os;
}

auto get_manhattan_dist(const Pos& p0, const Pos& p1) {
    return abs(p0.y - p1.y) + abs(p0.x - p1.x);
}

double get_euclidean_dist(const Pos& p0, const Pos& p1) {
    return sqrt((p0.y - p1.y)*(p0.y - p1.y) + (p0.x - p1.x)*(p0.x - p1.x));
}

enum class Dir {
    R,
    D,
    L,
    U
};

std::ostream& operator<<(std::ostream& os, Dir dir) {
    switch (dir) {
        case Dir::R:
            os << "R";
            break;
        case Dir::D:
            os << "D";
            break;
        case Dir::L:
            os << "L";
            break;
        case Dir::U:
            os << "U";
            break;
    }
    return os;
}

int dir2int(Dir dir) {
    if (dir == Dir::R) return 0;
    else if (dir == Dir::D) return 1;
    else if (dir == Dir::L) return 2;
    else if (dir == Dir::U) return 3;
    else assert(false);
}

Dir int2dir(int d) {
    if (d == 0) return Dir::R;
    else if (d == 1) return Dir::D;
    else if (d == 2) return Dir::L;
    else if (d == 3) return Dir::U;
    else assert(false);
}

Pos get_moved_pos(Pos p, int dir) {
    return Pos(p.y + dy[dir], p.x + dx[dir]);
}

pair<int, int> get_moved_pos(int y, int x, int dir) {
    return mp(y + dy[dir], x + dx[dir]);
}

Pos get_moved_pos(Pos p, Dir dir) {
    return get_moved_pos(p, dir2int(dir));
}

pair<int, int> get_moved_pos(int y, int x, Dir dir) {
    return get_moved_pos(y, x, dir2int(dir));
}