
#pragma once
#include <string>
#include <random>

class Module
{
public:
	
	static Module random(std::mt19937& gen, bool allow_prod);

	bool operator<(const Module& other)
	{
		if(name == "__")
			return false;
		if(other.name == "__")
			return true;
		return name > other.name;
	}

	double energy = 0;
	double speed = 0;
	double pollution = 0;
	double productivity = 0;
	int cost = 0; // This is an arbitrary cost so the optimizser prefers lower tiers of modules for a same amount of pollution
	std::string name;
};

class NoModule : public Module
{
public:
	NoModule()
	{
		name = "__";
	}
};

class EfficiencyModule1 : public Module
{
public:
	EfficiencyModule1()
	{
		energy = -0.3;
		name = "\u001b[32mE1\u001b[0m";
		cost = 1;
	}
};

class EfficiencyModule2 : public Module
{
public:
	EfficiencyModule2()
	{
		energy = -0.4;
		name = "\u001b[32mE2\u001b[0m";
		cost = 100;
	}
};

class EfficiencyModule3 : public Module
{
public:
	EfficiencyModule3()
	{
		energy = -0.5;
		name = "\u001b[32mE3\u001b[0m";
		cost = 10000;
	}
};

class SpeedModule1 : public Module
{
public:
	SpeedModule1()
	{
		energy = 0.5;
		speed = 0.2;
		name = "\u001b[34mS1\u001b[0m";
		cost = 1;
	}
};

class SpeedModule2 : public Module
{
public:
	SpeedModule2()
	{
		energy = 0.6;
		speed = 0.3;
		name = "\u001b[34mS2\u001b[0m";
		cost = 100;
	}
};

class SpeedModule3 : public Module
{
public:
	SpeedModule3()
	{
		energy = 0.7;
		speed = 0.5;
		name = "\u001b[34mS3\u001b[0m";
		cost = 10000;
	}
};

class ProductivityModule1 : public Module
{
public:
	ProductivityModule1()
	{
		energy = 0.4;
		speed = -0.05;
		productivity = 0.04;
		pollution = 0.05;
		name = "\u001b[31mP1\u001b[0m";
		cost = 1;
	}
};

class ProductivityModule2 : public Module
{
public:
	ProductivityModule2()
	{
		energy = 0.6;
		speed = -0.1;
		productivity = 0.06;
		pollution = 0.07;
		name = "\u001b[31mP2\u001b[0m";
		cost = 100;
	}
};

class ProductivityModule3 : public Module
{
public:
	ProductivityModule3()
	{
		energy = 0.8;
		speed = -0.15;
		productivity = 0.1;
		pollution = 0.1;
		name = "\u001b[31mP3\u001b[0m";
		cost = 10000;
	}
};
