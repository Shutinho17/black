#ifndef BLACK_DEBUG_RANDOM_MODELS_HPP
#define BLACK_DEBUG_RANDOM_MODELS_HPP

#include <black/support/common.hpp>
#include <black/logic/logic.hpp>
#include <random>

namespace black_internal::random {
  using namespace logic;
  
  // Struct for the single world, contains true propositions and suceesors
  struct world{
    std::vector<size_t> facts;
    std::vector<size_t> successors;
  };

  // Complete random model, this struct contains both the graph and the LTL
  // encoding, this will be helpful in applying the black solve function
  struct rand_model{
    std::vector<std::string> ap;
    std::vector<world> worlds;
    formula ltl_encoding;
  };

  BLACK_EXPORT
  rand_model random_model(
    std::mt19937& gen, alphabet& sigma, int nW, int nE,
    std::vector<std::string> const& symbols);
 
}

namespace black {
  using black_internal::random::world;
  using black_internal::random::rand_model;
  using black_internal::random::random_model;
}

#endif