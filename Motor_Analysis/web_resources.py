import requests
import json

global base_uri
base_uri = "https://raw.githubusercontent.com/germanespinosa/cellworld_data/master/"

def get_resource(resource_type, key0, *argv ):
    resource_uri = base_uri + resource_type + "/" + key0
    for arg in argv:
        resource_uri += "." + arg
    response = requests.get(resource_uri)
    return json.loads(response.text)
