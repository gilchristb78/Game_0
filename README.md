# Game_0
My first go at learning Unreal Engine 5

In the last two months I have begun to learn Unreal Engine 5, Following multiple tutorials and Courses.
This first project implements many basic systems and frameworks that I intend to improve and rewrite after learning a variety of tools and making basic games.

Systems:
- Building: allows the player to place and destroy actors within the world as well as use instanced meshes to reduce work loads
- Crafting: allows the player to craft items by trading existing items in their inventory for a new crafted item or items.
- Interacting: allows the player to interact with actors, creating a connection between the player and an actor, and example is opening a chest and viewing its inventory.
- Inventory: allows the player and other actors to hold items

Basics:
- Items: An  Item (and variants) that has mesh, texture, name, description, actions, rarity, weight, stackability, and other specifieers
- Recipes: A Recipe that has input items and output items to be used for crafting
- Item Spawn: A tool that can be used to spawn a bunch of items on the ground around this component
- Lootable: An actor that can be looted: ie. a chest that has an inventory and upon interacting with it you can take or leave items.
- Pickup: An Actor that is itself an item and can be pickedup off the "floor"


These sustems and basics together allow you to string together rootamentary gameplay where you can collect items, craft them together, build structures and more. it is far from a game but has afforded me the opportunity to learn the Unreal Engine Systems and will be improved in the next iterations
