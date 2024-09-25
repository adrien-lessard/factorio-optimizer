
#include "module.h"

Module Module::random(std::mt19937& gen, bool allow_prod)
{
	static auto with_prod = std::uniform_int_distribution<>(0, 9);
	static auto without_prod = std::uniform_int_distribution<>(0, 6);
	int r = allow_prod ? with_prod(gen) : without_prod(gen);

	switch(r)
	{
	case 0:
		return NoModule();
	case 1:
		return EfficiencyModule1();
	case 2:
		return EfficiencyModule2();
	case 3:
		return EfficiencyModule3();
	case 4:
		return SpeedModule1();
	case 5:
		return SpeedModule2();
	case 6:
		return SpeedModule3();
	case 7:
		return ProductivityModule1();
	case 8:
		return ProductivityModule2();
	case 9:
		return ProductivityModule3();
	}
	return NoModule();
}
