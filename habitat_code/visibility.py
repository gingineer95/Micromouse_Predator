import web_resources as wr
import map

class Visibility:
    def __init__(self, world):
        self.visibility = wr.get_resource("graph", world.name, "visibility")
        self.map = map.Map(world)

    def is_visible (self, coordinate1, coordinate2):
        id1 = self.map.cell(coordinate1)["id"]
        id2 = self.map.cell(coordinate2)["id"]
        if id2 in self.visibility[id1]:
            return True
        return False
