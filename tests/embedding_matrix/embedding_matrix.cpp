#include <black/logic/parser.hpp>
#include <black/logic/logic.hpp>
#include <black/solver/solver.hpp>
#include <black/internal/debug/random_formula.hpp>
#include <black/internal/debug/random_models.hpp>

#include <iostream>
#include <random>
#include <cstring>

using namespace black;

const int DEFAULT_DIM = 5;
const int DEFAULT_WRL = 10;
const int DEFAULT_HGT = 10;
const int DEFAULT_MDT = 5;
const int MAX_ITERATE = 1000;
const unsigned int DEFAULT_RNG = 17;

[[ noreturn ]] void help();
std::vector<std::string> parse_ap(std::string);

void help() {
  std::cerr
      << "\nGenerator for a semantica embedding matrix.\n"
      << "\nUsage embedding_matrix [options]\n"
      << "\nOptions:\n"
      << "\t--dim <x>   : Number of formulas and models wanted, must be x>0."
      << "\n\t          : Default is " << DEFAULT_DIM << "\n"
      << "\t--worlds <x>: Maximum number of worlds in each model, must be x>0"
      << "\n\t          : Default is " << DEFAULT_WRL << "\n"
      << "\t--height <x>: Maximum height of each formula, must be x>0."
      << "\n\t          : Default is " << DEFAULT_HGT << "\n"
      << "\t--md <x>    : Maximum modal depth of each formula, must be x>0."
      << "\n\t          : Default is " << DEFAULT_MDT << "\n"
      << "\t--ap<xs>    : Set of propositional given in the form '[p,q,...]'.\n"
      << "\n\t          : Default is a set '[p1,..,pn]' with n the log2 of the"
      << "\n\t          : dimension of the generated formual.\n"
      << "\t--seed <x>  : Integer number used to init the random generator."
      << "\n\t          : Default is " << DEFAULT_RNG << "\n";
  exit(1);
}

std::vector<std::string> parse_ap(std::string str_ap) {
  std::vector<std::string> ap;
  std::string p;

  if (str_ap.front() != '[' || str_ap.back() != ']') {
    std::cerr << "Malformed propositional set expression: " << str_ap << "\n";
    exit(1);
  }

  unsigned long i=1;
  while (i < str_ap.size()-1) {
    if (str_ap[i] == ',') {
      ap.push_back(p);
      p.clear();
    } else {
      if (str_ap[i] != ' ') p.push_back(str_ap[i]);
    }
    ++i;
  }

  if (!p.empty()) ap.push_back(p);

  return ap;
}

// Model checking operation on m AND !(f). If the tribool answer is true, then 
// it means that our model checking is false leading to a 0 in the matrix.
// If the tribool answer is false, the opposite occur, so 1 is returned.
int eval(black::formula f, rand_model m, scope& xi, solver& slv) {
  std::optional<black::formula> fs = m.ltl_encoding && !(f);
  black::tribool answer = slv.solve(xi, *fs, false);

  if (answer == true) {
    return 0;
  }
  return 1;
}

int main(int argc, char **argv) {
  std::mt19937 gen((std::random_device())());
  unsigned int s = DEFAULT_RNG;
  int dim = DEFAULT_DIM, worlds = DEFAULT_WRL;
  int height = DEFAULT_HGT, md = DEFAULT_MDT;
  int edges = DEFAULT_WRL;
  std::vector<std::string> ap;
  bool ap_set = false;

  // Parsing CLI arguments
  int i=1;
  while (i < argc) {
    if (std::strcmp(argv[i], "--dim") == 0) {
      try {
        dim = std::stoi(argv[++i]);
      } catch (const std::invalid_argument& e) {
        std::cerr << "Invalid argument: " << e.what() << "\n";
        exit(1);
      }
    } else if (std::strcmp(argv[i], "--worlds") == 0) {
      try {
        worlds = std::stoi(argv[++i]);
      } catch (const std::invalid_argument& e) {
        std::cerr << "Invalid argument: " << e.what() << "\n";
        exit(1);
      }
    } else if (std::strcmp(argv[i], "--height") == 0) {
      try {
        height = std::stoi(argv[++i]);
      } catch (const std::invalid_argument& e) {
        std::cerr << "Invalid argument: " << e.what() << "\n";
        exit(1);
      }
    } else if (std::strcmp(argv[i], "--md") == 0) {
      try {
        md = std::stoi(argv[++i]);
      } catch (const std::invalid_argument& e) {
        std::cerr << "Invalid argument: " << e.what() << "\n";
        exit(1);
      }
    } else if (std::strcmp(argv[i], "--seed") == 0) {
      try {
        s = std::stoul(argv[++i]);
      } catch (const std::invalid_argument& e) {
        std::cerr << "Invalid argument: " << e.what() << "\n";
        exit(1);
      }
    } else if (std::strcmp(argv[i], "--ap") == 0) {
      ap = parse_ap(argv[++i]);
      ap_set = true;
    } else {
      std::cerr << "Unknow argument: " << argv[i] << "\n";
      help();
    }
    ++i;
  }

  if (dim < 1 || worlds < 1 || height < 1 || md < 1) {
    // We could also reset to DEFAULT values
    std::cerr << "Non positivi arguments are not allowed.\n";
    help();
  }

  gen.seed(s);

  if (!ap_set && ap.empty()){
    for (int j=1; j<=(int)floor(log2(dim)); j++) {
      ap.push_back("p" + std::to_string(j));
    }
  }

  std::uniform_int_distribution<> distrib(dim, dim * dim);
  edges = distrib(gen);

  alphabet sigma;
  std::vector<black::formula> formulas;
  std::vector<black::rand_model> models;
  std::vector<std::vector<int>> matrix(dim, std::vector<int>(dim, 0));

  scope xi(sigma);
  solver slv;



  // naive implementation of matrix

  for (int i = 0; i < dim; ++i) {
    black::formula f = random_ltl_formula(gen, sigma, height, md, ap);
    formulas.push_back(f);
  }

  for (int i = 0; i < dim; ++i) {
    rand_model m = random_model(gen, sigma, worlds, edges, ap);
    models.push_back(m);
  }

  for (int i = 0; i < dim; ++i) {
    for (int j = 0; j < dim; ++j) {
      matrix[i][j] = eval(formulas[i], models[j], xi, slv);
    }
  }

  for (const auto& row : matrix) {
    for (int value : row) {
      std::cout << "\t" << value;
    }
    std::cout << std::endl;
  }
}