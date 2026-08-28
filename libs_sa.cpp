
// TODO部分を書き換える

// store pre-calc log probs before sa loop
class LogProb {
    static const int MAX_SIZE = 0x80000;
    array<double, MAX_SIZE> log_prob;

    public:
    LogProb() {
        for (auto& r : log_prob) r = log(rand01());
    }

    double get(int iteration) const {
        return log_prob[iteration & (MAX_SIZE-1)];
    }
};

LogProb log_probs;

///////////////////////////////////////////////////////////////////

template<typename T>
bool compare_best(T& best_score, const T& next_score) {
    if (Env::minimization) return chmin(best_score, next_score);
    else return chmax(best_score, next_score);
}

namespace SA {
    struct State {
    };

    int get_score(const State& state) {
        int score = 0;
        return score;
    }

    struct Solver {
        double progress_ratio = 0.0;
        double time = 0.0;
        double temp = 0.0;

        int print_interval = 10000;

        int iteration = 0;
        int last_best_solver = -1;

        int score;
        decltype(score) best_score;

        State state;
        State best_state;

        Solver(const State& init_state) {
            state = init_state;
            score = get_score(state);

            best_state = state;
            best_score = score;
        }

        template<typename T>
        void print_trans_stat(int trans_idx, T delta_score, double threshold) {
            if (iteration % print_interval == 0) {
                const auto next_score = score + delta_score;
                DEBUG("[time:{:.3f}], [temp:{:.4f}], [best_solver:{}], [trans:{}], [threshold:{:.4f}], [delta:{}], [score:{}]\n", time, temp, last_best_solver, trans_idx, threshold, delta_score, next_score);
            }
        }

        State solve(double end_time) {
            vector<int> n_trans_try(Env::n_transition);
            vector<int> n_trans_done(Env::n_transition);
            int n_best_update = 0;
            const auto sa_start = timer.get_time();

            for (iteration = 0; ; iteration++) {
                if ((iteration & 0xff) == 0) {
                    time = timer.get_time();
                    if(time > end_time) break;

                    progress_ratio = (time - sa_start) / (end_time - sa_start);
                    temp = get_exponential_interpolate(progress_ratio, Env::start_temp, Env::end_temp);
                }

                const double prob = rand01();
                const int trans = 0;
                ASSERT(trans < Env::n_transition, "trans = {}", trans);

                bool take_trans;
                if (trans == 0) {
                    take_trans = transition_template(trans);
                } else {
                    ASSERT(false, "trans = {}", trans);
                }

                n_trans_try[trans] += 1;

                if (take_trans) {
                    n_trans_done[trans] += 1;
                    if (compare_best(best_score, score)) {
                        /// update_best_solution
                        last_best_solver = trans;
                        best_state = state;
                        n_best_update += 1;
                    }
                }
            }

            REP(i, Env::n_transition) {
                DEBUG("trans {:2d}: {:9d} / {:9d} --> {:>6.3f}%\n", i, n_trans_done[i], n_trans_try[i], (n_trans_try[i] == 0) ? 0.0 : 100.0*n_trans_done[i]/n_trans_try[i]);
            }
            DEBUG("Iteration: {}, best_score: {}, n_update: {}\n", iteration, best_score, n_best_update);

            return best_state;
        }

        // TODO: copy this template and implement a transition function for each SA transition
        bool transition_template(int trans_idx) {
            const auto nex_score = get_score(state);
            auto delta_score = nex_score - score;
            const double threshold = log_probs.get(iteration) * temp * ((Env::minimization) ? -1 : 1);
            const bool force_next = (Env::minimization) ? delta_score < threshold : delta_score > threshold;

            print_trans_stat(trans_idx, delta_score, threshold);

            if (force_next) {
                // take changes
                score += delta_score;
                return true;
            } else {
                // discard changes
                return false;
            }
        }
    };
};
