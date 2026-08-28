
#ifdef ONLINE_JUDGE
#pragma GCC optimize "Ofast,omit-frame-pointer,inline,unroll-all-loops"
#define SUBMIT
#else
//#define SUBMIT
#endif

#include <bits/stdc++.h>

using namespace std;
using namespace chrono;

typedef long long ll;
typedef unsigned long long ull;
#define REP(i, n) for(int i=0; i<int(n); i++)
#define REPi(i, a, b) for(int i=int(a); i<int(b); i++)
#define MEMS(a,b) memset(a,b,sizeof(a))
#define mp make_pair
#define MOD(a, m) ((a % m + m) % m)
#define ALL(a) (a).begin(), (a).end()
#define RALL(a) (a).rbegin(), (a).rend()
template<class T>bool chmax(T &a, const T &b) { if (a<b) { a=b; return 1; } return 0; }
template<class T>bool chmin(T &a, const T &b) { if (b<a) { a=b; return 1; } return 0; }
constexpr ll MOD = 1e9+7;
constexpr ll INF = 1e14;

constexpr int SEED = 1000;

static mt19937 engine;

#ifdef AWS_CLOUD
#define AWS_TIME_SCALE 2.0  // TODO: adjust scale
#else
#define AWS_TIME_SCALE 1.0
#endif

namespace Env {
    constexpr double time_limit = AWS_TIME_SCALE * 1.950;
    constexpr double start_temp = 1000;
    constexpr double end_temp = 10;
    constexpr bool minimization = false;
    constexpr int n_transition = 1;
    constexpr int beam_width = 10;
};

#ifdef SUBMIT
#define DEBUG(...) ((void)0)
#define DUMP(...) ((void)0)
#define ASSERT(...) ((void)0)
#else
#define DEBUG(format_string, ...) \
    do { \
        ::debug_utils::print(std::cerr, format_string __VA_OPT__(,) __VA_ARGS__); \
    } while (false)
#define DUMP(var) \
    do { \
        ::debug_utils::dump(std::cerr, #var, (var)); \
    } while (false)
#define ASSERT(expr, fmt_str, ...) \
    do { \
        if (!(expr)) { \
            ::debug_utils::fail_assertion( \
                std::cerr, #expr, __FILE__, __LINE__, __func__, \
                fmt_str __VA_OPT__(,) __VA_ARGS__ \
            ); \
        } \
    } while (0)
#include "dbg_utils.h"
#endif // SUBMIT

/*
Library: https://github.com/kura197/HeuLibrary
*/

////////////////////////////////////////////////////////////////////

using time_point_t = std::chrono::_V2::system_clock::time_point;

// R, D, L, U
constexpr int dx[] = {1, 0, -1, 0};
constexpr int dy[] = {0, 1, 0, -1};

////////////////////////////////////////////////////////////////////

/// xor128
unsigned int randxor(){
    static unsigned int x=123456789, y=362436069, z=521288629, w=88675123;
    unsigned int t;
    t = (x^(x<<11));
    x = y;
    y = z;
    z = w; 
    return( w=(w^(w>>19))^(t^(t>>8)) );
}

/// return [0,1)
double rand01(){
    return 1.0 * randxor() / numeric_limits<unsigned int>::max();
}

/// return [left, right)
int rand_int(const int left, const int right){
    assert(right > left);
    return randxor() % (right - left) + left;
}

template<class T>
void shuffle(vector<T>& v) {
    int sz = v.size();
    for(int i = sz; i > 1; i--) {
        auto p = rand_int(0, i);
        swap(v[i-1], v[p]);
    }
}

template<class T>
inline T sample(const vector<T>& v) {
    ASSERT(v.size() > 0, "");
    return v[rand_int(0, v.size())];
}

////////////////////////////////////////////////////////////////////

struct Timer {
    std::chrono::_V2::system_clock::time_point sp;

    Timer() : sp(system_clock::now()) {}

    double get_time() const {
        const double t = duration_cast<microseconds>(system_clock::now() - sp).count() * 1e-6;
        return t;
    }
};

Timer timer;

////////////////////////////////////////////////////////////////////

template <std::floating_point T>
double get_linear_interpolate(T progress, double start_val, double end_val) {
    return start_val + (end_val - start_val) * progress;
}

template <std::floating_point T>
double get_exponential_interpolate(T progress, double start_val, double end_val) {
    return start_val * pow(end_val/start_val, progress);
}

array<int, 4> di;
void build_di(int width) {
    di[0] = 1;
    di[1] = width;
    di[2] = -1;
    di[3] = -width;
}

///////////////////////////////////////////////////////////////////

// TODO
namespace input {
    void read_input() {
    }
};

////////////////////////////////////////////////////////////////////

// TODO
struct Answer {
    Answer() {
    }

    void print_answer() {
    }
};

////////////////////////////////////////////////////////////////////

Answer solve(double end_time) {
    Answer ans;
    return ans;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]){
    ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    //engine = mt19937(SEED);

    //read_args(argc, argv);

    input::read_input();
    //build_di(input::N);
    Answer answer = solve(Env::time_limit);
    answer.print_answer();

    const double time = timer.get_time();
    DEBUG("time: {:.3f} [s]\n", time);
    //assert(time < 2.0);
    return 0;
}
