
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <memory>
#include <thread>
#include <format>
#include <csignal>
#include <random>

#include <nlohmann/json.hpp>

#include "module.h"

using json = nlohmann::json;
using namespace std;

constexpr int N_BEACON_MODULES = 12;

unordered_map<string, int> names_to_id;

class FactorySection
{
public:

	FactorySection(json& element, int element_id)
	{
		this->name = element["name"].get<string>();
		this->id = element_id;
		this->allow_prod = true;
		this->r_time = element["energy_required"].get<double>();
		this->r_yield = element["result_count"].get<double>();
		this->crafting_speed = element["crafting_speed"].get<double>();
		this->emissions_per_minute = element["emissions_per_minute"].get<double>();
		this->module_slots = element["module_slots"].get<int>();
		this->beacon_slots = N_BEACON_MODULES;
		this->modules.resize(this->module_slots, NoModule());
		this->beacons.resize(this->beacon_slots, NoModule());

		for (auto& ingredient : element["ingredients"])
		{
			double amount = ingredient["amount"].get<double>();
			int ingredient_id = names_to_id[ingredient["name"].get<string>()];
			if(ingredient_id == 0)
				missing_recipes.insert(ingredient["name"].get<string>());
			this->recipe.push_back({amount, ingredient_id});
		}

		distrib = std::uniform_int_distribution<>(0, module_slots + beacon_slots - 1);
	}

	static bool module_comparator(Module &a, Module &b)
	{
		if(a.name == "__")
			return false;
		if(b.name == "__")
			return true;
		return a.name > b.name;
	}

	string module_names()
	{
		std::sort(modules.begin(), modules.end(), module_comparator);
		string mstring;
		for(int i = 0; i < module_slots; i++)
			mstring += modules[i].name;
		for(int i = 0; i < 4 - module_slots; i++)
			mstring += "  ";
		return mstring;
	}

	string beacon_names()
	{
		std::sort(beacons.begin(), beacons.end(), module_comparator);
		string mstring;
		for(int i = 0; i < beacon_slots; i++)
			mstring += beacons[i].name;
		return mstring;
	}

	void compute_factors()
	{
		energy_factor = 1.0;
		speed_factor = 1.0;
		pollution_factor = 1.0;
		productivity_factor = 1.0;
		module_cost = 0;

		for(auto& module : modules)
		{
			energy_factor += module.energy;
			speed_factor += module.speed;
			pollution_factor += module.pollution;
			productivity_factor += module.productivity;
			module_cost += module.cost;
		}

		for(auto& beacon : beacons)
		{
			energy_factor += beacon.energy/2;
			speed_factor += beacon.speed/2;
			pollution_factor += beacon.pollution/2;
			productivity_factor += beacon.productivity/2;
			module_cost += beacon.cost;
		}

		if(energy_factor < 0.2)
			energy_factor = 0.2;
	}

	double single_recipe_time()
	{
		return r_time / speed_factor / 60 / crafting_speed;
	}

	double pollute(double n_recepies)
	{
		return n_recepies * single_recipe_time() * emissions_per_minute * energy_factor * pollution_factor;
	}

	double n_recipes(double qty_to_build)
	{
		return qty_to_build / r_yield / productivity_factor;
	}

	string name;
	int id;
	double r_time;
	double r_yield;
	double emissions_per_minute;
	int module_slots;
	int beacon_slots;
	double crafting_speed;
	vector<pair<double, int>> recipe;
	vector<Module> modules;
	vector<Module> beacons;
	int module_cost;
	bool allow_prod;
	double total_buildings = 0.0;
	double total_recipes = 0.0;

	double energy_factor = 1.0;
	double speed_factor = 1.0;
	double pollution_factor = 1.0;
	double productivity_factor = 1.0;

	std::uniform_int_distribution<> distrib;

	static set<string> missing_recipes;
};

set<string> FactorySection::missing_recipes;

class Solution
{
public:
	Solution()
	{

	}

	Solution(const Solution& sol)
		: items_to_build(sol.items_to_build), generated_pollution(sol.generated_pollution), module_costs(sol.module_costs), total_pollution(sol.total_pollution)
	{
		for(auto& e : factory_config)
			e.reset();
		factory_config.clear();

		for(auto& e : sol.factory_config)
			factory_config.push_back(std::make_unique<FactorySection>(*e));
	}

	static void swap(Solution& a, Solution& b)
	{
		std::swap(a.factory_config, b.factory_config);
		std::swap(a.items_to_build, b.items_to_build);
		std::swap(a.generated_pollution, b.generated_pollution);
		std::swap(a.module_costs, b.module_costs);
		std::swap(a.total_pollution, b.total_pollution);
	}

	Solution& operator=(const Solution& sol)
	{
		Solution s(sol);
		swap(*this, s);
		return *this;
	}

	virtual inline bool operator< (const Solution& other) { return false; }

	void start_build_order()
	{
		fill(items_to_build.begin(), items_to_build.end(), 0);
		fill(generated_pollution.begin(), generated_pollution.end(), 0);
		module_costs = 0;
		for(auto& f : factory_config)
			f->total_buildings = 0;
	}

	void random_op(std::mt19937& gen)
	{
		auto section_distrib = std::uniform_int_distribution<>(0, factory_config.size() - 1);
		auto& factorySection = factory_config[section_distrib(gen)];
		int total_slots = factorySection->module_slots + factorySection->beacon_slots;
		if(total_slots > 0)
		{
			int module_to_change = factorySection->distrib(gen);
			int beacon_to_change = module_to_change - factorySection->module_slots;

			if(beacon_to_change >= 0)
				factorySection->beacons[beacon_to_change] = Module::random(gen, false);
			else
				factorySection->modules[module_to_change] = Module::random(gen, factorySection->allow_prod);
		}
	}

	void add_build(int id, double qty)
	{
		auto& factorySection = factory_config[id];

		factorySection->compute_factors();

		double n_recepies = factorySection->n_recipes(qty);

		for(auto& ingredient : factorySection->recipe)
		{
			double req_qty = ingredient.first * n_recepies;
			add_build(ingredient.second, req_qty);
		}

		double buildings = n_recepies * factorySection->single_recipe_time();
		factorySection->total_buildings += buildings;
		factorySection->total_recipes += n_recepies;

		items_to_build[id] += qty; // TODO also have an array for number of recipes // TODO rename to units_built
		generated_pollution[id] += factorySection->pollute(n_recepies);
	}

	void end_build_order()
	{
		double req_ho = items_to_build[names_to_id["heavy-oil"]];
		double req_lo = items_to_build[names_to_id["light-oil"]];
		double req_pg = items_to_build[names_to_id["petroleum-gas"]];

		int aop_id = names_to_id["advanced-oil-processing"];
		int hoc_id = names_to_id["heavy-oil-cracking"];
		int loc_id = names_to_id["light-oil-cracking"];

		auto& aopSection = factory_config[aop_id];
		auto& hocSection = factory_config[hoc_id];
		auto& locSection = factory_config[loc_id];

		aopSection->compute_factors();
		hocSection->compute_factors();
		locSection->compute_factors();

		double c1 = 100, c2 = 55, c3 = 45, c4 = 25; // advanced oil processing
		double c5 =  30, c6 = 20; // light oil cracking
		double c7 = 40, c8 = 30; // heavy oil cracking

		c2 *= aopSection->productivity_factor;
		c3 *= aopSection->productivity_factor;
		c4 *= aopSection->productivity_factor;
		c6 *= locSection->productivity_factor;
		c8 *= hocSection->productivity_factor;

		double ratio_ho = (1) / (c4);
		double ratio_lo = (c7) / (c3*c7+c4*c8);
		double ratio_pg = (c5*c7) / (c2*c5*c7+c3*c6*c7+c4*c8*c6);

		// Determine number of recipes to do for each process
		double recipes_ho = req_ho * ratio_ho;
		double recipes_lo = (req_lo - c3 * recipes_ho) * ratio_lo;
		double recipes_pg = (req_pg - c2 * (recipes_ho + recipes_lo)) * ratio_pg;
		double recipes_refinery = recipes_ho + recipes_lo + recipes_pg;
		double converted_ho_to_lo = recipes_refinery * c4 - req_ho;
		double converted_lo_to_pg = recipes_refinery * c3 + converted_ho_to_lo * c8 / c7 - req_lo;
		double recipes_ho_crack = converted_ho_to_lo / c7;
		double recipes_lo_crack = converted_lo_to_pg / c5;

		// Craft each process
		add_build(aop_id, recipes_refinery * aopSection->productivity_factor);
		add_build(hoc_id, recipes_ho_crack * hocSection->productivity_factor);
		add_build(loc_id, recipes_lo_crack * locSection->productivity_factor);

		for(auto& factorySection : factory_config)
			module_costs += factorySection->module_cost;
		total_pollution = std::reduce(generated_pollution.begin(), generated_pollution.end());
	}

	void run_build(std::mt19937& gen, bool randomize = false)
	{
		start_build_order();
		if(randomize)
			random_op(gen);
		build_order();
		end_build_order();
	}

	void print()
	{
		cout << endl;
		cout << left << setw(32) << setfill(' ') << "\u001b[4mFactory Section\u001b[0m";
		cout << right << setw(32) << setfill(' ') << "\u001b[4mUnits Built\u001b[0m";
		cout << right << setw(32) << setfill(' ') << "\u001b[4mPollution\u001b[0m";
		cout << right << setw(32) << setfill(' ') << "\u001b[4mBuildings\u001b[0m";
		cout << right << setw(32) << setfill(' ') << "\u001b[4mModules\u001b[0m";
		cout << right << setw(32) << setfill(' ') << "\u001b[4mBeacons\u001b[0m";
		cout << endl;
		for(int id = 0; auto& recipes : items_to_build)
		{
			if(recipes > 0)
			{
				cout << left << setw(24) << setfill(' ') << factory_config[id]->name;
				cout << right << setw(24) << setfill(' ') << fixed << setprecision(4) << recipes;
				cout << right << setw(24) << setfill(' ') << fixed << setprecision(4) << generated_pollution[id];
				cout << right << setw(24) << setfill(' ') << fixed << setprecision(4) << factory_config[id]->total_buildings;
				cout << left << setw(17) << setfill(' ') << " " << factory_config[id]->module_names();
				cout << left << setw(16) << setfill(' ') << " " << factory_config[id]->beacon_names();
				cout << endl;
			}
			id++;
		}
	}

	virtual void build_order(){}

	vector<std::unique_ptr<FactorySection>> factory_config;
	vector<double> items_to_build;
	vector<double> generated_pollution;
	int module_costs = 0;
	double total_pollution = 0.0;
};

class AllSciencePollutionOptimizer : public Solution
{
public:
	virtual void build_order()
	{
		add_build(names_to_id["rocket-part"], 100);
		add_build(names_to_id["satellite"], 1);
		add_build(names_to_id["automation-science-pack"], 1000);
		add_build(names_to_id["logistic-science-pack"], 1000);
		add_build(names_to_id["chemical-science-pack"], 1000);
		add_build(names_to_id["military-science-pack"], 1000);
		add_build(names_to_id["production-science-pack"], 1000);
		add_build(names_to_id["utility-science-pack"], 1000);
	}

	virtual inline bool operator< (const AllSciencePollutionOptimizer& rhs) 
	{
		return total_pollution < rhs.total_pollution || (total_pollution == rhs.total_pollution && module_costs < rhs.module_costs);
	}
};

// Optimize for pullution
AllSciencePollutionOptimizer current_solution;
AllSciencePollutionOptimizer best_solution;
volatile bool stop_signal = false;

void signal_handler(int signal)
{
	stop_signal = true;
}

std::mutex solution_mutex;
void runner(const int thread_id, const int iterations, const int deviations_allowed)
{
	AllSciencePollutionOptimizer local_solution;
	AllSciencePollutionOptimizer local_best_solution;

	int deviations_remaining = deviations_allowed;

	std::random_device rd;
	std::mt19937 gen(rd());

	{
		const std::lock_guard<std::mutex> lock(solution_mutex);
		local_solution = best_solution;
		local_best_solution = best_solution;
	}

	for(int i = 0; i < iterations && !stop_signal; i++)
	{
		local_solution.run_build(gen, true);

		double pollution = local_solution.total_pollution;
		double best_pollution = local_best_solution.total_pollution;
		
		if(local_solution < local_best_solution)
		{
			local_best_solution = local_solution;
			deviations_remaining = deviations_allowed;
		}
		else
		{
			if(deviations_remaining == 0)
			{
				local_solution = local_best_solution;
				deviations_remaining = deviations_allowed;
			}
			else
			{
				deviations_remaining--;
			}
		}

		// Every once in a while, compare the thread solution to the global solution
		if(i % 1000 == 0)
		{
			const std::lock_guard<std::mutex> lock(solution_mutex);

			// Update global solution if we are better
			if(local_best_solution < best_solution)
			{
				cout << std::format("Iteration {} / {}. Thread {} got the best solution: {:.4f}", i, iterations, thread_id, local_best_solution.total_pollution) << std::endl;
				best_solution = local_best_solution;
			}

			// Follow the global solution if we are worse
			if(best_solution < local_best_solution)
			{
				cout << std::format("Thread {} resets its best solution", thread_id) << std::endl;
				local_best_solution = best_solution;
			}
		}
	}
}

int main()
{
	// Load recipes and entities
	ifstream ifs_recipe("recipe.json");
	ifstream ifs_entities("entities.json");
	if(ifs_recipe.fail() || ifs_entities.fail())
	{
		cout << "Run retrieve_lua.py to generate recipe.json and entities.json first." << endl;
		exit(0);
	}

	// Parse recipes and entities
	json j = json::parse(ifs_recipe);
	json j_entities = json::parse(ifs_entities);

	// Initialize the current solution
	current_solution.items_to_build.resize(j.size(), 0);
	current_solution.generated_pollution.resize(j.size(), 0);

	// Assign IDs to each recipe / entity
	for(int id = 0; auto& element : j)
		names_to_id.insert({element["name"].get<string>(), id++});

	// Initialize each factory section
	for(int id = 0; auto& element : j)
	{
		auto factorySection = std::make_unique<FactorySection>(element, id);
		current_solution.factory_config.emplace_back(std::move(factorySection));
		id++;
	}

	// Report missing recipes
	for(auto& m : FactorySection::missing_recipes)
		cout << "Missing recipe for " << m << endl;

	// Disallow prod modules for entities
	for(auto& element : j_entities)
	{
		auto id = names_to_id[element.get<string>()];
		if(id != 0)
			current_solution.factory_config[id]->allow_prod = false; // careful, inaccurate
	}

	// Basic solution witout optimization
	std::random_device rd;
	std::mt19937 gen(rd());
	current_solution.run_build(gen);
	best_solution = current_solution;

	std::signal(SIGINT, signal_handler);

	cout << "Done building list" << endl;

	// Accept deviations from the best solutions up to a certain point
	// This reduces the chances to get stuck on a local minimum
	int deviations_allowed = 10;

	// Optimization loop
	constexpr int iterations = 10'000'000;
	unsigned int n_threads = std::thread::hardware_concurrency();
	std::vector<std::unique_ptr<std::thread>> threads;
	for(int i = 0; i < n_threads; i++)
		threads.emplace_back(std::make_unique<std::thread>(&runner, i, iterations, deviations_allowed));
	for(auto& thread : threads)
	{
		thread->join();
		thread.reset();
	}

	best_solution.print();
	cout << endl << "Pollution: " << best_solution.total_pollution << " Modules: " << best_solution.module_costs << endl;

	return 0;
}

