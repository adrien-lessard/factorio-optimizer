
import wget, os, json

def download(url, file):
	if os.path.exists(file):
		os.remove(file)
	wget.download(url + file)

def recipe():
	lines = ['data = \n']
	with open('recipe.lua', 'r') as f:
		f.readline()
		for line in f:
			lines.append(line)
	lines.pop()
	lines.append('local lunajson = require \'lunajson\'\n')
	lines.append('print(lunajson.encode(data))\n')

	with open('recipe_formatted.lua', 'w') as f:
		for line in lines:
			f.write(line)

	os.system('lua recipe_formatted.lua > recipe.json')

	with open('recipe.json', 'r') as f:
		data = json.load(f)
		newdata = []

		newdata.append( {
			"enabled": False,
			"type": "recipe",
			"name": "not-found",
			"result": "not-found",
			"ingredients": [],
			"result_count": 1,
			"crafting_speed": 1,
			"emissions_per_minute": 0,
			"module_slots": 0,
			"energy_required": 0
		})

		newdata.append( {
			"enabled": False,
			"type": "recipe",
			"name": "advanced-oil-processing",
			"ingredients": [
				{
					"name": "crude-oil",
					"amount": 100
				},
				{
					"name": "water",
					"amount": 50
				}
			],
			"energy_required": 5,
			"category": "raw",
			"result_count": 1,
			"crafting_speed": 1,
			"emissions_per_minute": 6,
			"module_slots": 3
		})

		newdata.append( {
			"enabled": False,
			"type": "recipe",
			"name": "heavy-oil-cracking",
			"ingredients": [
				{
					"name": "heavy-oil",
					"amount": 40
				},
				{
					"name": "water",
					"amount": 30
				}
			],
			"energy_required": 2,
			"category": "raw",
			"result_count": 1,
			"crafting_speed": 1,
			"emissions_per_minute": 4,
			"module_slots": 3
		})

		newdata.append( {
			"enabled": False,
			"type": "recipe",
			"name": "light-oil-cracking",
			"ingredients": [
				{
					"name": "light-oil",
					"amount": 30
				},
				{
					"name": "water",
					"amount": 30
				}
			],
			"energy_required": 2,
			"category": "raw",
			"result_count": 1,
			"crafting_speed": 1,
			"emissions_per_minute": 4,
			"module_slots": 3
		})

		newdata.append( {
			"enabled": False,
			"type": "recipe",
			"name": "raw-fish",
			"ingredients": [],
			"energy_required": 1,
			"category": "raw",
			"result_count": 1,
			"crafting_speed": 1,
			"emissions_per_minute": 0,
			"module_slots": 0
		})

		newdata.append( {
			"enabled": False,
			"type": "recipe",
			"name": "wood",
			"ingredients": [],
			"energy_required": 1,
			"category": "raw",
			"result_count": 1,
			"crafting_speed": 1,
			"emissions_per_minute": 0,
			"module_slots": 0
		})

		newdata.append( {
			"enabled": False,
			"type": "recipe",
			"name": "water",
			"ingredients": [],
			"energy_required": 0.1,
			"category": "raw",
			"result_count": 120,
			"crafting_speed": 1,
			"emissions_per_minute": 0,
			"module_slots": 0
		})

		newdata.append( {
			"enabled": False,
			"type": "recipe",
			"name": "crude-oil",
			"ingredients": [],
			"energy_required": 0.5,
			"category": "raw",
			"result_count": 1,
			"crafting_speed": 1,
			"emissions_per_minute": 10,
			"module_slots": 2
		})

		newdata.append( {
			"enabled": False,
			"type": "recipe",
			"name": "heavy-oil",
			"ingredients": [],
			"energy_required": 0,
			"category": "raw",
			"result_count": 1,
			"crafting_speed": 1,
			"emissions_per_minute": 0,
			"module_slots": 0
		})

		newdata.append( {
			"enabled": False,
			"type": "recipe",
			"name": "light-oil",
			"ingredients": [],
			"energy_required": 0,
			"category": "raw",
			"result_count": 1,
			"crafting_speed": 1,
			"emissions_per_minute": 0,
			"module_slots": 0
		})

		newdata.append( {
			"enabled": False,
			"type": "recipe",
			"name": "petroleum-gas",
			"ingredients": [],
			"energy_required": 0,
			"category": "raw",
			"result_count": 1,
			"crafting_speed": 1,
			"emissions_per_minute": 0,
			"module_slots": 0
		})

		newdata.append( {
			"enabled": False,
			"type": "recipe",
			"name": "copper-ore",
			"ingredients": [],
			"energy_required": 2,
			"category": "raw",
			"result_count": 1,
			"crafting_speed": 1,
			"emissions_per_minute": 12,
			"module_slots": 2
		})

		newdata.append( {
			"enabled": False,
			"type": "recipe",
			"name": "iron-ore",
			"ingredients": [],
			"energy_required": 2,
			"category": "raw",
			"result_count": 1,
			"crafting_speed": 1,
			"emissions_per_minute": 12,
			"module_slots": 2
		})

		newdata.append( {
			"enabled": False,
			"type": "recipe",
			"name": "stone",
			"ingredients": [],
			"energy_required": 2,
			"category": "raw",
			"result_count": 1,
			"crafting_speed": 1,
			"emissions_per_minute": 12,
			"module_slots": 2
		})

		newdata.append( {
			"enabled": False,
			"type": "recipe",
			"name": "coal",
			"ingredients": [],
			"energy_required": 2,
			"category": "raw",
			"result_count": 1,
			"crafting_speed": 1,
			"emissions_per_minute": 12,
			"module_slots": 2
		})

		for item in data:

			# Remove expensive recipes
			if 'expensive' in item:
				del item['expensive']
				for key, val in item['normal'].items():
					item[key] = val
				del item['normal']

			# Get the time required to build
			if 'energy_required' not in item:
				item['energy_required'] = 0.5

			# Get type of factory
			if 'category' not in item or item['category'] == 'crafting-with-fluid' or item['category'] == 'advanced-crafting':
				item['category'] = 'crafting'

			# Always use solid fuel from light oil
			if item['name'] == 'solid-fuel-from-light-oil':
				item['name'] = 'solid-fuel'

			# Flush recipes with more than one result
			if 'results' in item:
				if len(item['results']) == 1:
					try:
						for element in item['results']:
							item['result'] = element['name']
							item['result_count'] = element['amount']
							del item['results']
					except TypeError:
						print('deleting', item['name'])
						continue
				else:
					print('deleting', item['name'])
					continue
			if item['result'] != item['name']:
				print('deleting', item['name'])
				continue
			del item['result']
			if 'result_count' not in item:
				item['result_count'] = 1
			formatted_ingredients = []
			for ingredient in item['ingredients']:
				try:
					formatted_ingredients.append({'name' : ingredient['name'], 'amount' : ingredient['amount']})
				except:
					formatted_ingredients.append({'name' : ingredient[0], 'amount' : ingredient[1]})
			item['ingredients'] = formatted_ingredients
			try: del item['crafting_machine_tint']
			except: pass

			if item['category'] == 'crafting': # assuming assembling-machine-3
				item['crafting_speed'] = 1.25
				item['emissions_per_minute'] = 2
				item['module_slots'] = 4
			if item['category'] == 'chemistry': # assuming chemical-plant
				item['crafting_speed'] = 1
				item['emissions_per_minute'] = 4
				item['module_slots'] = 3
			if item['category'] == 'smelting': # assuming electric-furnace
				item['crafting_speed'] = 2
				item['emissions_per_minute'] = 1
				item['module_slots'] = 2
			if item['category'] == 'rocket-building': # assuming rocket-silo
				item['crafting_speed'] = 1
				item['emissions_per_minute'] = 0
				item['module_slots'] = 4
			if item['category'] == 'centrifuging':
				print('deleting', item['name'])
				continue

			newdata.append(item)

	with open('recipe.json', 'w') as f:
		f.write(json.dumps(newdata, indent=4))

def entities():
	lines = ['data = \n']
	valid_entities = list()
	with open('entities.lua', 'r') as f:
		name = ''
		for line in f:
			if line.startswith('    name = '):
				try:
					name = line.split('"')[1]
					#print(name)
				except IndexError:
					pass
			if line.startswith('    collision_box'):
				valid_entities.append(name)
	valid_entities.append('satellite')
	valid_entities.append('piercing-rounds-magazine')
	valid_entities.append('firearm-magazine')
	valid_entities.append('grenade')
	valid_entities.append('rail')
	#print(valid_entities)
	with open('entities.json', 'w') as f:
		f.write(json.dumps(valid_entities, indent=4)) # careful, inaccurate

# Download data from web
download('https://raw.githubusercontent.com/wube/factorio-data/master/base/prototypes/', 'recipe.lua')
download('https://raw.githubusercontent.com/wube/factorio-data/master/base/prototypes/entity/', 'entities.lua')
download('https://raw.githubusercontent.com/wube/factorio-data/master/base/prototypes/entity/', 'resources.lua')
download('https://raw.githubusercontent.com/wube/factorio-data/master/base/prototypes/entity/', 'mining-drill.lua')

# Generate recipe file
recipe()

# Generate entity list
entities()
