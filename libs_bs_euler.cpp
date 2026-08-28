
// TODO部分を埋める

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
template<typename T, size_t MaxLog>
struct Trace {
    using Entry = pair<T, int>;
    alignas(Entry) array<unsigned char, sizeof(Entry) * MaxLog> log_storage;
    unsigned int log_size;

    Trace() : log_size(0) {}

    ~Trace() { clear(); }

    Trace(const Trace&) = delete;
    Trace& operator=(const Trace&) = delete;

    void clear() {
        if constexpr (!is_trivially_destructible_v<Entry>) {
            for (unsigned int i = 0; i < log_size; i++) {
                destroy_at(entry_ptr(i));
            }
        }
        log_size = 0;
    }

    unsigned int add(T c, int id) {
        ASSERT(log_size < MaxLog, "Trace log capacity exceeded: size={}, capacity={}", log_size, MaxLog);
        construct_at(entry_ptr(log_size), c, id);
        const unsigned int new_id = log_size;
        log_size++;
        return new_id;
    }

    vector<T> get(int id) {
        int len = 0;
        for (int cur = id; cur != -1; cur = entry(cur).second) {
            ASSERT(0 <= cur && cur < (int)log_size, "invalid trace id={}, size={}", cur, log_size);
            len++;
        }

        vector<T> ret(len);
        while (id != -1) {
            const auto& [c, nex] = entry(id);
            ret[--len] = c;
            id = nex;
        }
        return ret;
    }

    // return N-th ancestor node id to check beamsearch diversity
    int get_ancestor_id(int id, int nth) const {
        int cnt = 0;
        while (id != -1) {
            ASSERT(0 <= id && id < (int)log_size, "invalid trace id={}, size={}", id, log_size);
            id = entry(id).second;
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

private:
    Entry* entry_ptr(unsigned int id) {
        return std::launder(reinterpret_cast<Entry*>(log_storage.data() + sizeof(Entry) * id));
    }

    const Entry* entry_ptr(unsigned int id) const {
        return std::launder(reinterpret_cast<const Entry*>(log_storage.data() + sizeof(Entry) * id));
    }

    Entry& entry(unsigned int id) {
        return *entry_ptr(id);
    }

    const Entry& entry(unsigned int id) const {
        return *entry_ptr(id);
    }
};

/////////////////////////////////////////////

// オイラーツアー型差分更新ビームサーチ
// 使用例: https://atcoder.jp/contests/ahc052/submissions/69223239

template <class Key>
struct FastHashSet {
    explicit FastHashSet(uint32_t n) { init(n); }

    // n を次の 2 の冪に丸める
    void init(uint32_t n) {
        n_ = 1u;
        while (n_ < n) n_ <<= 1;         // power of two
        mask_ = n_ - 1;
        used_generation_.assign(n_, 0);
        keys_.assign(n_, Key{});
        size_ = 0;
        generation_ = 1;
    }

    // 既に存在すれば false、新規挿入なら true
    bool insert(Key key) {
        auto [found, idx] = find_slot(key);
        if (found) return false;
        used_generation_[idx] = generation_;
        keys_[idx] = key;
        ++size_;
        return true;
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
    std::pair<bool,uint32_t> find_slot(Key key) const {
        uint32_t i = static_cast<uint32_t>(key) & mask_;  // Key が良い分布と仮定
        while (used_generation_[i] == generation_) {
            if (keys_[i] == key) return {true, i};
            i = (i + 1) & mask_;
        }
        return {false, i}; // 空バケットの位置
    }

    uint32_t n_{0}, mask_{0}, size_{0}, generation_{1};
    std::vector<uint32_t> used_generation_;
    std::vector<Key> keys_;
};

struct DynamicBeamWidthConfig {
    // ===== User-tunable parameters =====

    // Upper/lower bounds for the beam width used in candidate selection.
    static constexpr int min_beam_width = 1;
    static constexpr int max_beam_width = 20;

    // Ignore very small observations because they are too noisy to fit.
    static constexpr int min_observed_count = 15;

    // Safety factor for per-turn time budget.
    static constexpr double safety_normal = 0.95;
    static constexpr double safety_last_10_turns = 0.98;
    static constexpr double safety_last_3_turns = 1.00;

    // Limit per-turn beam width changes to avoid oscillation.
    static constexpr double max_scale_up_normal = 1.5;
    static constexpr double max_scale_up_last_5_turns = 2.0;
    static constexpr double max_scale_down = 0.55;

    // Print one compact line per dynamic-width update.
    static constexpr bool debug_log = true;

    // ===== Advanced Kalman filter parameters =====
    // Initial estimate for per-expanded-candidate time in:
    //   turn_time ~= initial_a * expanded_count + initial_b
    static constexpr double initial_a = 1e-6;

    // Initial estimate for fixed per-turn overhead.
    static constexpr double initial_b = 0.0;

    // Initial uncertainty of initial_a. Larger values adapt faster at startup.
    static constexpr double initial_p00 = 1e-8;

    // Initial uncertainty of initial_b. Larger values adapt faster at startup.
    static constexpr double initial_p11 = 1e-6;

    // Process noise for initial_a, relative to the current a.
    // Larger values follow changing per-candidate cost more aggressively.
    static constexpr double q_a_rel = 0.02;

    // Process noise for fixed overhead b.
    // Larger values follow changing per-turn overhead more aggressively.
    static constexpr double q_b_abs = 1e-7;

    // Minimum observation noise. Larger values make updates smoother.
    static constexpr double r_abs = 1e-6;

    // Relative observation noise. Larger values trust each measurement less.
    static constexpr double r_rel = 0.35;

    // Clamp observed time to [prediction * low, prediction * high] before update.
    // This prevents one heavy/light turn from breaking the estimate.
    static constexpr double outlier_clip_low = 0.25;
    static constexpr double outlier_clip_high = 4.0;
};

struct BeamTimeKalman {
    // turn_time ~= a * expanded_count + b
    bool initialized = false;

    double a = DynamicBeamWidthConfig::initial_a;
    double b = DynamicBeamWidthConfig::initial_b;

    double p00 = DynamicBeamWidthConfig::initial_p00, p01 = 0.0;
    double p10 = 0.0, p11 = DynamicBeamWidthConfig::initial_p11;

    static constexpr double q_a_rel = DynamicBeamWidthConfig::q_a_rel;
    static constexpr double q_b_abs = DynamicBeamWidthConfig::q_b_abs;
    static constexpr double r_abs = DynamicBeamWidthConfig::r_abs;
    static constexpr double r_rel = DynamicBeamWidthConfig::r_rel;

    void initialize_from_observation(double expanded_count, double observed_time) {
        expanded_count = std::max(1.0, expanded_count);
        observed_time = std::max(1e-9, observed_time);

        a = std::max(1e-12, observed_time / expanded_count);
        b = 0.0;

        p00 = std::max(1e-18, a * a * 4.0);
        p01 = 0.0;
        p10 = 0.0;
        p11 = std::max(1e-10, observed_time * observed_time);

        initialized = true;
    }

    double predict_time(double expanded_count) const {
        expanded_count = std::max(1.0, expanded_count);
        const double pred = a * expanded_count + b;
        if (!std::isfinite(pred)) return 1e-9;
        return std::max(1e-9, pred);
    }

    void try_update(double expanded_count, double observed_time) {
        expanded_count = std::max(1.0, expanded_count);
        observed_time = std::max(1e-9, observed_time);

        if (!initialized) {
            initialize_from_observation(expanded_count, observed_time);
            return;
        }

        const double pred_before = predict_time(expanded_count);
        observed_time = std::clamp(
            observed_time,
            pred_before * DynamicBeamWidthConfig::outlier_clip_low,
            pred_before * DynamicBeamWidthConfig::outlier_clip_high
        );

        const double q_a = std::max(1e-18, a * a * q_a_rel * q_a_rel);
        const double q_b = q_b_abs;
        p00 += q_a;
        p11 += q_b;

        const double h0 = expanded_count;
        const double h1 = 1.0;
        const double y_pred = h0 * a + h1 * b;
        const double err = observed_time - y_pred;

        const double base = std::max(1e-9, std::max(observed_time, y_pred));
        const double r = std::max(r_abs, base * base * r_rel * r_rel);

        const double s =
            h0 * (p00 * h0 + p01 * h1) +
            h1 * (p10 * h0 + p11 * h1) +
            r;
        if (s <= 1e-18 || !std::isfinite(s)) return;

        const double k0 = (p00 * h0 + p01 * h1) / s;
        const double k1 = (p10 * h0 + p11 * h1) / s;
        if (!std::isfinite(k0) || !std::isfinite(k1)) return;

        a += k0 * err;
        b += k1 * err;
        if (!std::isfinite(a)) a = 1e-6;
        if (!std::isfinite(b)) b = 0.0;
        a = std::max(a, 1e-12);
        b = std::max(b, 0.0);

        const double np00 = (1.0 - k0 * h0) * p00 - k0 * h1 * p10;
        const double np01 = (1.0 - k0 * h0) * p01 - k0 * h1 * p11;
        const double np10 = -k1 * h0 * p00 + (1.0 - k1 * h1) * p10;
        const double np11 = -k1 * h0 * p01 + (1.0 - k1 * h1) * p11;

        p00 = std::max(np00, 1e-18);
        p11 = std::max(np11, 1e-18);

        const double off = 0.5 * (np01 + np10);
        p01 = off;
        p10 = off;
    }

    int estimate_beam_width(double target_time) const {
        target_time = std::max(1e-9, target_time);
        if (!initialized) return 1;
        if (target_time <= b) return 1;

        const double estimated_count = (target_time - b) / std::max(a, 1e-12);
        if (!std::isfinite(estimated_count)) return 1;
        return std::max(1, (int)std::floor(estimated_count));
    }
};

namespace BS {
    using OP = uint8_t;

    struct Node {
        OP op;
        int id;
        int rank;
    };

    struct Cand {
        OP op;
        int raw_score;
        int eval_score;
        int par;
        uint32_t hash;
        bool ok = false;
        bool fin = false;   // true if current state is acceptable.

        Cand() {}

        Node to_node(int id, int rank) {
            return Node{.op=op, .id=id, .rank=rank};
        }
    };

    struct State {
        // TODO

        State() {
        }

        void apply(const Node& node) {
            // TODO
        }

        void revert(const Node& node) {
            // TODO
        }
    };

    struct BeamSearch {
        static const int reserve_path_size = 1e5;   // ビーム幅 * alpha ?
        static const int reserve_cand_size = 1e5;   // ビーム幅 * 次の手数
        static const int reserve_tour_size = 1e5;   // ビーム幅 * beta ?
        static const int reserve_trace_size = 1e7;  // ビーム幅 * depth

        State state;
        vector<Cand> cand, next_cand;
        vector<Node> tour, next_tour;
        inline static Trace<OP, reserve_trace_size> trace;  // Note: cannot use multi instances
        vector<Node> path;

        BeamSearch(State init_state) : state(init_state) {
            trace.clear();

            cand.reserve(reserve_cand_size);
            next_cand.reserve(reserve_cand_size);
            tour.reserve(reserve_tour_size);
            next_tour.reserve(reserve_tour_size);
            path.reserve(reserve_path_size);

            Cand init_cand;
            // TODO: raw_scoreの初期値
            init_cand.op = OP{};
            init_cand.raw_score = 0;
            init_cand.eval_score = 0;
            init_cand.par = -1;
            init_cand.hash = 0;
            init_cand.ok = true;
            init_cand.fin = false;
            cand.emplace_back(init_cand);
        }

        // 現在の state から可能な操作を列挙する
        void push_cands(int cand_id, int par_id) {
            const auto& c = cand[cand_id];
            // TODO: next_candに次の候補をpush
            REP(n, 10) {
                // example
                Cand nc = c;
                nc.par = par_id;
                nc.op = n;
                nc.ok = false;
                nc.raw_score = 0;
                nc.eval_score = nc.raw_score*100000 + rand_int(0, 2);
                nc.hash = 0;
                nc.fin = false;
                next_cand.emplace_back(nc);
            }
        }

        void enum_cands() {
            next_cand.clear();

            if (tour.size() == 0) {
                Node root {.op = OP{}, .id = -1, .rank = 0};
                tour.emplace_back(root);
                push_cands(0, root.id);
                swap(next_cand, cand);
                return;
            }

            auto move_state = [&](const Node& pre_node, const Node& nex_node) {
                if (pre_node.rank >= nex_node.rank) {
                    //DEBUG("move {}({}) --> {}({})\n", pre_node.id, cur_rank, tour[ni].id, nex_rank);
                    state.revert(pre_node);
                }
                if (pre_node.rank <= nex_node.rank) {
                    //DEBUG("move {}({}) --> {}({})\n", pre_node.id, pre_node.rank, nex_node.id, nex_node.rank);
                    state.apply(nex_node);
                }
            };

            next_tour.clear();
            const int leaf_rank = tour[0].rank;
            int ni = tour.size()-1;
            for (int i = cand.size()-1; i >= 0; i--) {
                if (!cand[i].ok) continue;

                if (cand[i].par != tour[ni].id) {
                    path.clear();
                    path.emplace_back(tour[ni]);
                    while (1) {
                        ni -= 1;
                        ASSERT(ni >= 0, "ni = {}, i = {}, cand[i].par = {}, tour.size = {}", ni, i, cand[i].par, tour.size());
                        if (cand[i].par == tour[ni].id) break;
                        if (tour[ni].rank == leaf_rank) continue;
                        if (tour[ni].id == path.back().id) path.pop_back();
                        else path.emplace_back(tour[ni]);
                    }
                    path.emplace_back(tour[ni]);

                    REP(k, path.size()-1) {
                        const auto& pre_node = path[k];
                        const auto& nex_node = path[k+1];
                        move_state(pre_node, nex_node);
                    }

                    if (next_cand.size() > 0) {
                        next_tour.insert(next_tour.end(), ALL(path));
                    }
                }

                const int id = trace.add(cand[i].op, tour[ni].id);
                const auto node = cand[i].to_node(id, tour[ni].rank+1);
                state.apply(node);
                push_cands(i, node.id);
                state.revert(node);
                next_tour.emplace_back(node);
            }

            ASSERT(!next_tour.empty(), "next_tour is empty");

            //DEBUG("move to {}\n", next_tour.back().id);
            state.apply(next_tour.back());

            swap(next_cand, cand);
            swap(next_tour, tour);
        }

        vector<OP> get_ops(int id) {
            return trace.get(id);
        }

        vector<OP> solve(double end_time) {
            enum_cands();   // 1手目候補を生成

            vector<int> cand_idx;

            unordered_set<uint32_t> seen_hash;
            //FastHashSet<uint32_t> seen_hash(Env::beam_width * input::K * input::M);

            constexpr bool use_sorted_list = false;
            constexpr bool use_dynamic_beam_width = false;

            // TODO:
            const int max_turn = 100;

            int beam_width = Env::beam_width;
            BeamTimeKalman bw_kalman;

            for (int turn = 1; turn <= max_turn; turn++) {
                const double current_time = timer.get_time();
                if (current_time > end_time) { 
                    DEBUG("time exceeded\n");
                    return vector<OP>{};
                }

                DEBUG("turn = {}\n", turn);
                bool finish = false;
                seen_hash.clear();
                cand_idx.resize(cand.size());
                iota(ALL(cand_idx), 0);
                int selected_count = 0;

                if constexpr (use_sorted_list) {
                    // {eval_score, idx}
                    SortedList<pair<int, int>, less<pair<int, int>>> sorted_list(beam_width);  // TODO
                    for (const auto& idx : cand_idx) {
                        if (cand[idx].fin) { // 終了条件
                            cand[idx].ok = true;
                            finish = true;
                        }

                        if (!sorted_list.can_insert({cand[idx].eval_score, idx})) continue;
                        //if (seen_hash.contains(cand[idx].hash)) continue;
                        //seen_hash.insert(cand[idx].hash);
                        sorted_list.insert({cand[idx].eval_score, idx});
                    }

                    for (const auto& [_, idx] : sorted_list.get_list()) {
                        cand[idx].ok = true;
                        selected_count++;
                    }
                } else {
                    sort(ALL(cand_idx), [&](const auto& lhs, const auto& rhs){
                        return cand[lhs].eval_score < cand[rhs].eval_score;;  // TODO
                    });

                    int n_take = 0;
                    for (const auto& idx : cand_idx) {
                        if (cand[idx].fin) { // 終了条件
                            cand[idx].ok = true;
                            finish = true;
                        }
                        if (seen_hash.contains(cand[idx].hash)) continue;

                        bool update = false;
                        // TODO: 一段階目の重複削除を実装

                        if (update) {
                            cand[idx].ok = true;
                            n_take += 1;
                            seen_hash.insert(cand[idx].hash);
                            if (n_take == beam_width) break;
                        }
                    }

                    // 重複削除をしてもなお残ったcandをビーム幅まで採択する
                    for (const auto& idx : cand_idx) {
                        if (n_take == beam_width) break;
                        if (cand[idx].ok) continue;
                        if (seen_hash.contains(cand[idx].hash)) continue;
                        seen_hash.insert(cand[idx].hash);
                        cand[idx].ok = true;
                        n_take += 1;
                    }
                    selected_count = n_take;
                }

                if (finish) break;

                if (turn < max_turn) enum_cands();

                const double turn_end = timer.get_time();
                const double observed_time = turn_end - current_time;

                if constexpr (use_dynamic_beam_width) {
                    const int observed_count = selected_count;
                    const int required_count = min(beam_width, DynamicBeamWidthConfig::min_observed_count);
                    const bool reliable_for_kalman = observed_count >= required_count && observed_time > 0.0;
                    const bool had_kalman_prediction = bw_kalman.initialized;
                    const double pred_time_before_update = had_kalman_prediction
                        ? bw_kalman.predict_time((double)observed_count)
                        : -1.0;
                    const double pred_error_before_update = had_kalman_prediction
                        ? observed_time - pred_time_before_update
                        : 0.0;

                    if (reliable_for_kalman) {
                        bw_kalman.try_update((double)observed_count, observed_time);
                    }

                    const int remaining_turns = max_turn - turn;
                    const double remaining_time = end_time - turn_end;

                    if (remaining_turns > 0 && remaining_time > 0.0 && bw_kalman.initialized) {
                        double safety = DynamicBeamWidthConfig::safety_normal;
                        if (remaining_turns <= 10) {
                            safety = DynamicBeamWidthConfig::safety_last_10_turns;
                        }
                        if (remaining_turns <= 3) {
                            safety = DynamicBeamWidthConfig::safety_last_3_turns;
                        }

                        const double target_time = safety * remaining_time / remaining_turns;
                        const int estimated_beam_width = bw_kalman.estimate_beam_width(target_time);

                        const double max_scale_up =
                            remaining_turns <= 5
                                ? DynamicBeamWidthConfig::max_scale_up_last_5_turns
                                : DynamicBeamWidthConfig::max_scale_up_normal;

                        const int lo = max(
                            DynamicBeamWidthConfig::min_beam_width,
                            (int)floor(beam_width * DynamicBeamWidthConfig::max_scale_down)
                        );
                        const int hi = min(
                            DynamicBeamWidthConfig::max_beam_width,
                            max(lo, (int)ceil(beam_width * max_scale_up))
                        );
                        const int next_beam_width = std::clamp(estimated_beam_width, lo, hi);

                        if constexpr (DynamicBeamWidthConfig::debug_log) {
                            DEBUG(
                                "[BW] turn={} time={:.6f} pred_err={:.6f} a={:.3e} b={:.3e} next={}\n",
                                turn,
                                observed_time,
                                pred_error_before_update,
                                bw_kalman.a,
                                bw_kalman.b,
                                next_beam_width
                            );
                        }

                        beam_width = next_beam_width;
                    }
                }
            }

            vector<int> valid_idx;
            for (const auto& idx : cand_idx) {
                if (cand[idx].ok && cand[idx].fin) {    // only finished cands
                    valid_idx.push_back(idx);
                }
            }

            if (valid_idx.empty()) {
                DEBUG("no valid candidate found\n");
                return vector<OP>{};
            }

            // TODO: スコア最小化？
            const auto best_idx = *min_element(ALL(valid_idx), [&](const auto& lhs, const auto& rhs){
                return cand[lhs].raw_score < cand[rhs].raw_score;
            });
            const auto& best_cand = cand[best_idx];
            const int id = trace.add(best_cand.op, best_cand.par);
            const auto ops = get_ops(id);
            return ops;
        }
    };
} // namespace BS
