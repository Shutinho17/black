#include <black/internal/debug/random_models.hpp>
#include <black/logic/logic.hpp>

#include <variant>
#include <random>
#include <array>
#include <algorithm>
#include <queue>
#include <numeric>

namespace black_internal::random{

  class random_model_gen
  {

  public:
    random_model_gen(
      std::mt19937& gen, alphabet& sigma,
      const std::vector<std::string>& symbols
    ) : _gen{gen}, _sigma{sigma}, _ap{symbols}
    { }

    rand_model random_model(int nW, int nE);

    private:
      std::mt19937& _gen;
      alphabet& _sigma;
      std::vector<std::string> _ap;

      world random_world(){
        world w;
        std::uniform_int_distribution<> dist(0, 1);
        for (size_t i = 0; i < _ap.size(); ++i) {
          if(dist(_gen)){
            w.facts.push_back(i);
          }
        }
        return w;
      }

      void generate_successors(std::vector<world>& worlds, int nW, int nE) {
        size_t indexW1, indexW2;
        std::uniform_int_distribution<size_t> dist(0, worlds.size() - 2);
        for (size_t i = 0; i < (size_t)nW; ++i){
          indexW2 = dist(_gen);
          
          // no self-loop in this first successor's generation
          if (indexW2 >= i){
            ++indexW2;
          }
          
          worlds[i].successors.push_back(indexW2);
        }

        dist = std::uniform_int_distribution<size_t>(0, worlds.size() - 1);
        int edges_counter = nW;
        while (edges_counter < nE) {
          indexW1 = dist(_gen);
          indexW2 = dist(_gen);

          auto &succ = worlds[indexW1].successors;
          bool is_old_edge =
            std::find(succ.begin(), succ.end(), indexW2) != succ.end();
          
          if (!is_old_edge) {
            succ.push_back(indexW2);
            ++edges_counter;
          }
          
        }
      }

    std::vector<size_t> bfs(std::vector<world> worlds, size_t init) {
      std::vector<bool> visited(worlds.size(), false);
      std::queue<int> q;
      std::vector<size_t> reachable;


      visited[init] = true;
      reachable.push_back(init);
      q.push(init);

      while (!q.empty()) {
        int curr = q.front();
        q.pop();

        for (size_t s : worlds[curr].successors) {
          if(!visited[s]) {
            visited[s] = true;
            reachable.push_back(s);
            q.push(s);
          }
        }
      }
      
      return reachable;
    }

    // This function transform a random model in a LTL formula, at the start 
    // a random world get chosen as initial state world (init) and then 
    // everything gets encoded as a conjuction of disjunctions, implications 
    // and tomorrows
    formula model_encoder(std::vector<world> worlds){
      size_t init;
      std::vector<size_t> reachable;
      std::uniform_int_distribution<size_t> distr(0, worlds.size() - 1);
      std::vector<size_t> index(_ap.size());
      std::vector<std::pair<size_t, size_t>> pairs;

      init = distr(_gen);
      reachable = bfs(worlds, init);

      for (size_t i = 0; i < reachable.size(); ++i) {
        for (size_t j = i + 1; j < reachable.size(); ++j) {
          pairs.push_back({reachable[i], reachable[j]});
        }
      }

      // Denoting each world with a string like w + integer (w0, w1, w2, ...)
      auto w = [&](size_t i) -> formula {
        return _sigma.proposition("w" + std::to_string(i));
      };
 
      // This helps in reducing the dimension of the final formula, more 
      // precisely it helps in excluding all the situation in which it seems 
      // that we are in 2 worlds at the same time
      formula world_exclusion = big_and(_sigma, pairs, [&](auto pr) -> formula {
        return !(w(pr.first) && w(pr.second));
      });

      std::iota(index.begin(), index.end(), size_t{0});

      // Main lmabda function, for each world we start by creating the 
      // conjuction of disjunction. Then we establish which are the propositions
      // in the set _ap which are true in a said world. 
      // Lastly we combine everything implying possible future worlds from the 
      // current one along with true props. 
      formula each_world = big_and(_sigma, reachable, [&](size_t i) -> formula {
        formula next = big_or(_sigma, worlds[i].successors,
          [&](size_t j) -> formula {
            return w(j);
          });

        auto const& facts = worlds[i].facts;
        formula f_world = big_and(_sigma, index, [&](size_t k) -> formula {
          bool is_prop_true = std::find(
              facts.begin(), facts.end(), k) != facts.end();
          if (is_prop_true){
            return _sigma.proposition(_ap[k]);
          } else {
            return formula{!_sigma.proposition(_ap[k])};
          }
        });

        return implies(w(i), X(next)) && implies(w(i), f_world);
      });

      return w(init) && G(world_exclusion && each_world);
    }

  };    

  rand_model random_model_gen::random_model(int nW, int nE) {
    std::vector<world> worlds;

    for (int i = 0; i < nW ; ++i) {
      worlds.push_back(random_world());
    }

    generate_successors(worlds, nW, nE);

    formula ltl_encoding = model_encoder(worlds);

    return rand_model{_ap, worlds, ltl_encoding};
  }

  rand_model random_model(
    std::mt19937& gen, alphabet& sigma, int nW, int nE,
    std::vector<std::string> const& symbols) {
      return random_model_gen{gen, sigma, symbols}.random_model(nW, nE);
  }
}

