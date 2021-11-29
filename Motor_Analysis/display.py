import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
from world import World
from visibility import Visibility


class Display:
    def __init__ (self, heat_map, cmap = "Reds"):
        self.heat_map = heat_map
        self.fig, self.ax = plt.subplots(1, 1, figsize=(10, 10))
        self.im = self.ax.imshow(self.heat_map.values, cmap=cmap, interpolation='nearest')
        self.ax.axes.xaxis.set_visible(False)
        self.ax.axes.yaxis.set_visible(False)

    def add_line(self, coord1, coord2, color="red", arrow=False):
        x = coord1["x"] + 7
        y = -coord1["y"] + 7
        dx = coord2["x"] - coord1["x"]
        dy = -(coord2["y"] - coord1["y"])
        self.ax.arrow(x, y, dx, dy,
                  head_width=0.2 if arrow else 0,
                  width=0.05,
                  color=color,
                  fill=True,
                  length_includes_head=True)

    def add_patch(self, location, color):
        return self.ax.add_patch(Rectangle((location[1] - .5, location[0] - .5), 1, 1, fill=True, color=color, lw=0))

    def add_outline(self, location, color):
        return self.ax.add_patch(Rectangle((location[1] - .5, location[0] - .5), 1, 1, fill=False, edgecolor=color, lw=2))

    def add_text(self, location, text):
        return self.ax.text(location[1] - .5, location[0] - .5, text)

    def add_occlusions(self, color):
        for cell in self.heat_map.world.cells:
            if cell["occluded"]:
                self.add_patch(self.heat_map.get_index(cell["coordinates"]), color)

    def add_squares (self, color):
        for cell in self.heat_map.world.cells:
            self.add_outline(self.heat_map.get_index(cell["coordinates"]), color)

    def save(self, file_name):
        self.fig.savefig(file_name)

    def show(self):
        plt.show()

    def close(self):
        plt.close()
