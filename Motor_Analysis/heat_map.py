import numpy as np
from world import World

class Heat_map:
  def __init__ (self, world):
    self.world = world
    self.size = (world.dimensions["height"], world.dimensions["width"])
    self.values = np.zeros(self.size)
    self.minx = world.coordinates[0]["x"]
    self.miny = world.coordinates[0]["y"]


  def get_index(self, coordinates):
    return (coordinates["y"] - self.miny, coordinates["x"] - self.minx)
