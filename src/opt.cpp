
#include <fstream>
#include <iostream>
#include <map>
#include <set>

#include <nlohmann/json.hpp>

#include "module.h"
#include "optimizer.h"

using json = nlohmann::json;
using namespace std;

constexpr int N_BEACON_MODULES = 12;

unordered_map<string, int> names_to_id;

class FactorySection
{
public:

	FactorySection(json& element, int element_id)
	{
		name = element["name"].get<string>();
		id = element_id;
		allow_prod = true;
		r_time = element["energy_required"].get<double>();
		r_yield = element["result_count"].get<double>();
		crafting_speed = element["crafting_speed"].get<double>();
		emissions_per_minute = element["emissions_per_minute"].get<double>();
		module_slots = element["module_slots"].get<int>();
		beacon_slots = N_BEACON_MODULES;
		modules.resize(module_slots, NoModule());
		beacons.resize(beacon_slots, NoModule());

		for (auto& ingredient : element["ingredients"])
		{
			double amount = ingredient["amount"].get<double>();
			int ingredient_id = names_to_id[ingredient["name"].get<string>()];
			if(ingredient_id == 0)
				missing_recipes.insert(ingredient["name"].get<string>());
			recipe.push_back({amount, ingredient_id});
		}

		distrib = std::uniform_int_distribution<>(0, module_slots + beacon_slots - 1);
	}

	string get_module_names()
	{
		std::sort(modules.begin(), modules.end());
		string mstring;
		for(int i = 0; i < module_slots; i++)
			mstring += modules[i].name;
		for(int i = 0; i < 4 - module_slots; i++)
			mstring += "  ";
		return mstring;
	}

	string get_beacon_names()
	{
		std::sort(beacons.begin(), beacons.end());
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
		module_costs = 0;

		for(auto& module : modules)
		{
			energy_factor += module.energy;
			speed_factor += module.speed;
			pollution_factor += module.pollution;
			productivity_factor += module.productivity;
			module_costs += module.cost;
		}

		for(auto& beacon : beacons)
		{
			energy_factor += beacon.energy/2;
			speed_factor += beacon.speed/2;
			pollution_factor += beacon.pollution/2;
			productivity_factor += beacon.productivity/2;
			module_costs += beacon.cost;
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
	bool allow_prod;
	double r_time;
	double r_yield;
	double crafting_speed;
	double emissions_per_minute;
	int module_slots;
	int beacon_slots;

	vector<Module> modules;
	vector<Module> beacons;

	vector<pair<double, int>> recipe;

	double module_costs = 0;
	double buildings = 0;
	double recipes = 0;
	double pollution = 0;
	double units_built = 0;

	double energy_factor = 1;
	double speed_factor = 1;
	double pollution_factor = 1;
	double productivity_factor = 1;

	std::uniform_int_distribution<> distrib;

	static set<string> missing_recipes;
};

set<string> FactorySection::missing_recipes;

class Solution
{
public:
	Solution() { }
	virtual ~Solution()
	{
		for(auto& e : factory_config)
			e.reset();
	}

	Solution(const Solution& sol)
		: total_module_costs(sol.total_module_costs), total_buildings(sol.total_buildings), total_recipes(sol.total_recipes), total_pollution(sol.total_pollution), total_units_built(sol.total_units_built)
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
		std::swap(a.total_module_costs, b.total_module_costs);
		std::swap(a.total_buildings, b.total_buildings);
		std::swap(a.total_recipes, b.total_recipes);
		std::swap(a.total_pollution, b.total_pollution);
		std::swap(a.total_units_built, b.total_units_built);
	}

	Solution& operator=(const Solution& sol)
	{
		Solution s(sol);
		swap(*this, s);
		return *this;
	}

	virtual inline bool operator< (const Solution& rhs)
	{
		return get_metric() < rhs.get_metric();
	}
	virtual inline double get_metric() const { return -1; }

	void start_build_order()
	{
		for(auto& f : factory_config)
		{
			f->module_costs = 0;
			f->buildings = 0;
			f->recipes = 0;
			f->pollution = 0;
			f->units_built = 0;
		}

		total_module_costs = 0;
		total_buildings = 0;
		total_recipes = 0;
		total_pollution = 0;
		total_units_built = 0;
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

		// Number of buildings increases due to beacons
		int building_multiplier = 1;
		for(auto& b : factorySection->beacons)
			if(b.name != "__")
				building_multiplier++;

		factorySection->buildings += n_recepies * factorySection->single_recipe_time() * building_multiplier;
		factorySection->recipes += n_recepies;
		factorySection->pollution += factorySection->pollute(n_recepies);
		factorySection->units_built += qty;
	}

	void end_build_order()
	{
		double req_ho = factory_config[names_to_id["heavy-oil"]]->units_built;
		double req_lo = factory_config[names_to_id["light-oil"]]->units_built;
		double req_pg = factory_config[names_to_id["petroleum-gas"]]->units_built;

		int aop_id = names_to_id["advanced-oil-processing"];
		int hoc_id = names_to_id["heavy-oil-cracking"];
		int loc_id = names_to_id["light-oil-cracking"];

		auto& aopSection = factory_config[aop_id];
		auto& hocSection = factory_config[hoc_id];
		auto& locSection = factory_config[loc_id];

		aopSection->compute_factors();
		hocSection->compute_factors();
		locSection->compute_factors();

		double c2 = 55, c3 = 45, c4 = 25; // advanced oil processing
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

		for(auto& f : factory_config)
			total_module_costs += f->module_costs;
		for(auto& f : factory_config)
			total_buildings += f->buildings;
		for(auto& f : factory_config)
			total_recipes += f->recipes;
		for(auto& f : factory_config)
			total_pollution += f->pollution;
		for(auto& f : factory_config)
			total_units_built += f->units_built;
	}

	void compute()
	{
		start_build_order();
		build_order();
		end_build_order();
	}

	void print()
	{
		cout << endl;
		cout << left << setw(32) << setfill(' ') << "\u001b[4mFactory Section\u001b[0m";
		cout << right << setw(32) << setfill(' ') << "\u001b[4mUnits Built\u001b[0m";
		cout << right << setw(32) << setfill(' ') << "\u001b[4mRecipes Completed\u001b[0m";
		cout << right << setw(32) << setfill(' ') << "\u001b[4mPollution\u001b[0m";
		cout << right << setw(32) << setfill(' ') << "\u001b[4mBuildings\u001b[0m";
		cout << right << setw(32) << setfill(' ') << "\u001b[4mModules\u001b[0m";
		cout << right << setw(32) << setfill(' ') << "\u001b[4mBeacons\u001b[0m";
		cout << endl;
		for(int id = 0; auto& f : factory_config)
		{
			if(f->units_built > 0)
			{
				cout << left << setw(24) << setfill(' ') << f->name;
				cout << right << setw(24) << setfill(' ') << fixed << setprecision(4) << f->units_built;
				cout << right << setw(24) << setfill(' ') << fixed << setprecision(4) << f->recipes;
				cout << right << setw(24) << setfill(' ') << fixed << setprecision(4) << f->pollution;
				cout << right << setw(24) << setfill(' ') << fixed << setprecision(4) << f->buildings;
				cout << left << setw(17) << setfill(' ') << " " << f->get_module_names();
				cout << left << setw(16) << setfill(' ') << " " << f->get_beacon_names();
				cout << endl;
			}
			id++;
		}
	}

	virtual void build_order(){}

	vector<std::unique_ptr<FactorySection>> factory_config;

	double total_module_costs = 0;
	double total_buildings = 0;
	double total_recipes = 0;
	double total_pollution = 0;
	double total_units_built = 0;
};

class AllSciencePollutionOptimizer : public Solution
{
public:
	virtual ~AllSciencePollutionOptimizer() {}
	virtual void build_order()
	{
		double SPM = 50;
		add_build(names_to_id["rocket-part"], SPM / 10);
		add_build(names_to_id["satellite"], SPM / 1000);
		add_build(names_to_id["automation-science-pack"], SPM);
		add_build(names_to_id["logistic-science-pack"], SPM);
		add_build(names_to_id["chemical-science-pack"], SPM);
		add_build(names_to_id["military-science-pack"], SPM);
		add_build(names_to_id["production-science-pack"], SPM);
		add_build(names_to_id["utility-science-pack"], SPM);
	}

	virtual inline bool operator< (const Solution& rhs)
	{
		return total_pollution < rhs.total_pollution || (total_pollution == rhs.total_pollution && total_module_costs < rhs.total_module_costs);
	}

	virtual inline double get_metric()
	{
		return total_pollution;
	}
};

class FootprintOptimizer : public Solution
{
public:
	virtual ~FootprintOptimizer() {}
	virtual void build_order()
	{
		double SPM = 50;
		add_build(names_to_id["rocket-part"], SPM / 10);
		add_build(names_to_id["satellite"], SPM / 1000);
		add_build(names_to_id["automation-science-pack"], SPM);
		add_build(names_to_id["logistic-science-pack"], SPM);
		add_build(names_to_id["chemical-science-pack"], SPM);
		add_build(names_to_id["military-science-pack"], SPM);
		add_build(names_to_id["production-science-pack"], SPM);
		add_build(names_to_id["utility-science-pack"], SPM);
	}

	virtual inline bool operator< (const Solution& rhs)
	{
		return total_buildings < rhs.total_buildings || (total_buildings == rhs.total_buildings && total_module_costs < rhs.total_module_costs);
	}

	virtual inline double get_metric()
	{
		return total_buildings;
	}
};

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

	// Assign IDs to each recipe / entity
	for(int id = 0; auto& element : j)
		names_to_id.insert({element["name"].get<string>(), id++});

	// Initialize each factory section
	AllSciencePollutionOptimizer problem;
	for(int id = 0; auto& element : j)
	{
		auto factorySection = std::make_unique<FactorySection>(element, id);
		problem.factory_config.emplace_back(std::move(factorySection));
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
			problem.factory_config[id]->allow_prod = false; // careful, inaccurate
	}

	Optimizer<AllSciencePollutionOptimizer> solver(problem);
	AllSciencePollutionOptimizer solution = solver.run(1000000, 10);

	solution.print();

	cout << endl;
	cout << "module_costs: " << solution.total_module_costs << endl;
	cout << "buildings: " << solution.total_buildings << endl;
	cout << "recipes: " << solution.total_recipes << endl;
	cout << "pollution: " << solution.total_pollution << endl;
	cout << "units_built: " << solution.total_units_built << endl;
	cout << endl;
	return 0;
}

