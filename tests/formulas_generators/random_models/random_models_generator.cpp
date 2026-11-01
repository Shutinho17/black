#include <black/logic/parser.hpp>
#include <black/logic/prettyprint.hpp>
#include <black/internal/debug/random_models.hpp>

#include <iostream>
#include <functional>
#include <random>
#include <variant>
#include <cstring>

using namespace black;

enum class logic_t { ltl, ltlp };

const int MIN_WORLD = 10;
const int MAX_WORLD = 20;

/* ---------------------------------- MAIN ---------------------------------- */
[[ noreturn ]] void help();
std::vector<std::string> parse_ap(std::string);

void help() {
  std::cerr
			<< "\nGenerator for random models.\n"
			<< "\nUsage random_models_generator [options]\n"
			<< "\nOptions:\n"
			<< "\t--num <x>		: Number of models wanted, must be x>0. Default is 1\n"
			<< "\t--dimW <x>	: Maximum number of worlds in each model, must be x>0."
			<< "\n\t					: Default is a random number between " << MIN_WORLD
			<< " and " << MAX_WORLD << "\n"
			<< "\t--dimE <x>	: Maximum number of edges in each model, must be x>dimW"
			<< " and x<(dimW * dimW) \n"
			<< "\t						: Default is a random number between dimW e its square"
			<< "\n"
			<< "\t--ap <xs>		: Set of propositional given in the form '[p,q,..]'.\n"
      << "\t            : Default is a set '[p1,..,pn]' with n the log2 of the"
      << "\n"
      << "\t            : number of worlds in each model.\n"
			<< "\t --seed <x>	: Integer number used to init the random generator.\n";
	exit(1);
}

std::vector<std::string> parse_ap(std::string str_ap) {
	std::vector<std::string> ap;
	std::string p;

	if (str_ap.front() != '[' || str_ap.back() != ']')  {
		std::cerr << "Malformed propositionals set expression: " << str_ap << "\n";
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

/* Produce the set of random generated models
 */
int main(int argc, char **argv) {
  std::mt19937 gen((std::random_device())());
	std::uniform_int_distribution<> distribW(MIN_WORLD, MAX_WORLD);

	unsigned int s = 0;
	int num = 1, dimW = MIN_WORLD, dimE = MIN_WORLD;
	std::vector<std::string> ap;
	bool ap_set = false, edge_specified = false;

	int i=1;
	while (i < argc) {
		if (std::strcmp(argv[i], "--num") == 0) {
      try {
        num = std::stoi(argv[++i]);
      } catch (const std::invalid_argument& e) {
        std::cerr << "Invalid argument: " << e.what() << "\n";
        exit(1);
      }
    } else if (std::strcmp(argv[i], "--seed") == 0) {
      try {
        s = (uint32_t) std::stoul(argv[++i]);
      } catch (const std::invalid_argument& e) {
        std::cerr << "Invalid argument: " << e.what() << "\n";
        exit(1);
      }
      gen.seed(s);
      dimW = distribW(gen); // need to re-generate the dimension
		} else if (std::strcmp(argv[i], "--dimW") == 0) {
      try {
        dimW = std::stoi(argv[++i]);
      } catch (const std::invalid_argument& e) {
        std::cerr << "Invalid argument: " << e.what() << "\n";
        exit(1);
      }
    } else if (std::strcmp(argv[i], "--dimE") == 0) {
			try {
				dimE = std::stoi(argv[++i]);
				edge_specified = true;
			} catch (const std::invalid_argument& e) {
				std::cerr << "Invalid argument: " << e.what() << "\n";
				exit(1);
			}
		} else if (std::strcmp(argv[i], "--ap") == 0) {
      ap = parse_ap(argv[++i]);
      ap_set = true;
		} else {
      std::cerr << "Unknown argument: " << argv[i] << "\n";
      help();
    }
    ++i;	
	}

	if (num < 1 || dimW < 1 || dimE < 1) {
		std::cerr << "Non positive argument are not allowed. \n";
		help();
	}

	if ((dimE < dimW) || (dimE > (dimW * dimW))) {
		std::cerr 
			<< "Invalid number of edges. This should always be between the number of " 
			<< "worlds and its square \n";
		help();
	}

	if (!ap_set && ap.empty()) {
		for (int j=1; j<=(int)floor(log2(dimW)); j++){
			ap.push_back("p" + std::to_string(j));
		}
	}

	if (!edge_specified) {
		std::uniform_int_distribution<> distribE(MIN_WORLD, dimW * dimW);
		dimE = distribE(gen);
	}

	alphabet sigma;
	std::vector<black::rand_model> models;
	
	for (int i=0; i<num; i++){
		black::rand_model r = 
			black_internal::random::random_model(gen, sigma, dimW, dimE, ap);
		models.push_back(r);
	}

	for (const auto &r : models){
    std::cout << to_string(r.ltl_encoding) << "\n";
  }
	
	return 0;
}

