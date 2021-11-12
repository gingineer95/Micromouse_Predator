import web_resources as wr

class World:
    def __init__(self, world_name):
        world_json = wr.get_resource("world", world_name)
        self.name = world_json["name"]
        self.cells = world_json["cells"]
        minx, maxx = self.cells[0]["coordinates"]["x"], self.cells[0]["coordinates"]["x"]
        miny, maxy = self.cells[0]["coordinates"]["y"], self.cells[0]["coordinates"]["x"]
        for cell in self.cells:
            if cell["coordinates"]["x"] < minx:
                miny = cell["coordinates"]["x"]
            if cell["coordinates"]["x"] > maxx:
                maxx = cell["coordinates"]["x"]
            if cell["coordinates"]["y"] < miny:
                miny = cell["coordinates"]["y"]
            if cell["coordinates"]["y"] > maxy:
                maxy = cell["coordinates"]["y"]
        self.dimensions = {"width": 1 + maxx - minx, "height": 1 + maxy - miny}
        self.coordinates = [{"x": minx, "y": miny}, {"x": maxx, "y": maxy}]
