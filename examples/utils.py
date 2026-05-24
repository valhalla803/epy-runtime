import requests

def mainn():
    resp = requests.post("https://scentair.com/shop.html")
    print(resp.text)