
// 主にTODO部分を書き換える

// maintain bottom/top-K values. T must support less<T> for bottom-K and greater<T> for top-K.
template<class T, class Compare = less<T>>
struct SortedList {
    priority_queue<T, vector<T>, Compare> que;
    unsigned int size_limit;

    SortedList(unsigned int size) : size_limit(size) {}

    bool can_insert(T x) const {
        return que.size() < size_limit || Compare()(x, que.top());
    }

    void insert(T x) {
        if (!can_insert(x)) return;
        que.emplace(x);
        if (que.size() > size_limit) que.pop();
    }

    vector<T> get_list() {
        vector<T> ret;
        while (!que.empty()) {
            const auto x = que.top();
            que.pop();
            ret.emplace_back(x);
        }
        reverse(ALL(ret));
        // sorted by increasing order for bottom-K, while decreasing order for top-K
        return ret;
    }

    size_t size() const {
        return que.size();
    }
};

///////////////////////////////////////////////////////////////////

// store global state for beamsearch. Each state has their own ids.
template<typename T>
struct Trace {
    vector<pair<T, int>> log;

    Trace() {}

    unsigned int add(T c, int id) {
        log.emplace_back(c, id);
        const unsigned int new_id = log.size() - 1;
        return new_id;
    }

    vector<T> get(int id) {
        vector<T> ret;
        while (id != -1) {
            const auto& [c, nex] = log[id];
            ret.emplace_back(c);
            id = nex;
        }
        reverse(ALL(ret));
        return ret;
    }

    // return N-th ancestor node id to check beamsearch diversity
    int get_ancestor_id(int id, int nth) const {
        int cnt = 0;
        while (id != -1) {
            id = log[id].second;
            cnt++;
            if (cnt == nth) break;
        }
        return id;
    }

    // return histgram of ancestor node id of ids vector. to check beamsearch diversity
    unordered_map<int, int> get_hist_ancestor_id(const vector<int>& ids, int nth) const {
        unordered_map<int, int> hist;
        for (const auto& id : ids) {
            const int pid = get_ancestor_id(id, nth);
            hist[pid] += 1;
        }
        return hist;
    }
};

////////////////////////////////////////////////////////////////////

template <class Key, class Val>
struct FastHashMap {
    explicit FastHashMap(uint32_t n) { init(n); }

    // n を次の 2 の冪に丸める
    void init(uint32_t n) {
        n_ = 1u;
        while (n_ < n) n_ <<= 1;         // power of two
        mask_ = n_ - 1;
        used_generation_.assign(n_, 0);
        keys_.assign(n_, Key{});
        vals_.assign(n_, Val{});         // 値用配列の初期化
        size_ = 0;
        generation_ = 1;
    }

    // 既に存在すれば false、新規挿入なら true
    // (値は引数の val で設定される)
    bool insert(Key key, Val val) {
        auto [found, idx] = find_slot(key);
        if (found) return false;
        used_generation_[idx] = generation_;
        keys_[idx] = key;
        vals_[idx] = val;                // 値を保存
        ++size_;
        return true;
    }

    // 配列添字アクセス (存在しなければデフォルト構築して参照を返す)
    Val& operator[](Key key) {
        auto [found, idx] = find_slot(key);
        if (!found) {
            used_generation_[idx] = generation_;
            keys_[idx] = key;
            vals_[idx] = Val{};          // デフォルト値で初期化
            ++size_;
        }
        return vals_[idx];
    }

    // キーが存在すれば値へのポインタを、なければ nullptr を返す
    // (書き換え可能)
    Val* get(Key key) {
        auto [found, idx] = find_slot(key);
        if (found) return &vals_[idx];
        return nullptr;
    }

    // const版 get
    const Val* get(Key key) const {
        auto [found, idx] = find_slot(key);
        if (found) return &vals_[idx];
        return nullptr;
    }

    bool contains(Key key) const {
        auto [found, idx] = find_slot(key);
        (void)idx;
        return found;
    }

    void clear() {
        generation_++;
        if (generation_ == 0) {
            std::fill(used_generation_.begin(), used_generation_.end(), 0);
            generation_ = 1;
        }
        size_ = 0;
    }

    uint32_t size() const { return size_; }
    uint32_t capacity() const { return n_; }

private:
    // used_[i] == 0: 空, 1: 占有
    std::pair<bool, uint32_t> find_slot(Key key) const {
        uint32_t i = static_cast<uint32_t>(key) & mask_;
        while (used_generation_[i] == generation_) {
            if (keys_[i] == key) return {true, i};
            i = (i + 1) & mask_;
        }
        return {false, i}; // 空バケットの位置
    }

    uint32_t n_{0}, mask_{0}, size_{0}, generation_{1};
    std::vector<uint32_t> used_generation_;
    std::vector<Key> keys_;
    std::vector<Val> vals_; // 値を格納するベクタを追加
};

Answer solve_bs(const double end_time) {
    struct BSState {
        int score;
        int id;
        //uint32_t hash;

        BSState() : score(0), id(-1) {
        }
    };

    // score, state-idx
    using S = tuple<int, int>;

    vector<BSState> states[2];
    states[0].emplace_back(BSState());

    // values for final answer
    using T = int;
    Trace<T> trace;

    //// unordered_map<uint32_t, int> hash_score;
    //FastHashMap<uint32_t, int> hash_score(Env::beam_width);   // TODO: use hash_score if using hash function

    const int DEPTH = 100;  // TODO: change DEPTH
    REP(n, DEPTH) {
        SortedList<S, less<S>> cand(Env::beam_width);
        REP(i, states[0].size()) {
            // TODO: insert transitions to cand
            const auto& state = states[0][i];
            int score = state.score + 0;
            if (!cand.can_insert(make_tuple(score, i))) continue;
            // NOTE: results with the same hash and lower score can remain in cand
            //const auto nex_hash = state.hash;
            //if (hash_score.count(nex_hash) > 0 && hash_score[nex_hash] <= score) continue;
            cand.insert(make_tuple(score, i));
        }

        states[1].clear();
        for (const auto& [score, idx] : cand.get_list()) {
            BSState nex_state = states[0][idx];
            nex_state.score = score;
            nex_state.id = trace.add(0, nex_state.id);  // TODO: insert action to trace
            states[1].emplace_back(move(nex_state));
        }

        swap(states[0], states[1]);
    }

    const auto& best_state = states[0][0];
    DEBUG("finish beam search. best score = {}, id = {}\n", best_state.score, best_state.id);
    const auto trace_list = trace.get(best_state.id);

    Answer ans;
    // use trace_list to get ans
    return ans;
}
