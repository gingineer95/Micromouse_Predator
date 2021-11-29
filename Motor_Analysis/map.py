import numpy as np

class Map:
    def __init__ (self, world):
        self.world = world
        self._index = -np.ones((world.dimensions["width"], world.dimensions["height"]), np.integer)
        origin = world.coordinates[0]
        index = 0
        for cell in world.cells:
            self._index[(cell["coordinates"]["x"]-origin["x"], cell["coordinates"]["y"]-origin["y"])] = index
            index += 1

    def find (self, coordinates):
        return self._index[(coordinates["x"]-self.world.coordinates[0]["x"], coordinates["y"]-self.world.coordinates[0]["y"])]

    def cell(self, coordinates):
        return self.world.cells[self.find(coordinates)]