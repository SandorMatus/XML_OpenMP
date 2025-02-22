import xml.etree.ElementTree as ET
import random
import string
import os

# Expanded data for random selection
labels = [
    "Wireless Mouse", "Gaming Keyboard", "Bluetooth Headphones", 
    "USB-C Hub", "Mechanical Keyboard", "Webcam", 
    "Monitor Stand", "Laptop Stand", "Mouse Pad", "External Hard Drive",
    "Gaming Chair", "Portable SSD", "Smartphone Stand", 
    "USB Flash Drive", "HDMI Cable", "Docking Station",
    "Wireless Charger", "Webcam Cover", "Noise Cancelling Headphones",
    "Smartwatch", "Fitness Tracker"
]

stockrooms = [
    "Main Warehouse", "Secondary Warehouse", "Tech Storage", 
    "Accessories Shelf", "Electronics Corner",
    "Outlet Store", "Returns Processing", "Bulk Storage", 
    "Display Area", "Online Fulfillment Center"
]

types = [
    "Electronics", "Accessories", "Peripherals", 
    "Furniture", "Wearables", "Networking"
]

suppliers = [
    "Tech Supplies Inc.", "Gamer Gear Ltd.", "Office Essentials Co.", 
    "Gadget World", "Electro Depot", "Smart Tech Solutions", 
    "Digital Universe", "Innovative Gadgets", "High-Tech Supplies", 
    "Future Electronics", "Value Tech Products"
]

# Ensure the output directory exists
output_dir = "xml_files"
os.makedirs(output_dir, exist_ok=True)

# Generate 400,000 XML files
for i in range(1, 400001):
    # Create the root element
    stock_item = ET.Element("stockItem")
    
    # Create the item element
    item = ET.SubElement(stock_item, "item")
    
    # Generate unique name
    unique_name = f"item{str(i).zfill(6)}"  # item000001, item000002, ..., item1000000
    ET.SubElement(item, "uniquename").text = unique_name
    
    # Randomly select other item details
    ET.SubElement(item, "label").text = random.choice(labels)
    ET.SubElement(item, "stockroom").text = random.choice(stockrooms)
    ET.SubElement(item, "price").text = f"{random.uniform(10.0, 100.0):.2f}"  # Random price between 10.00 and 100.00
    ET.SubElement(item, "new_price").text = f"{random.uniform(5.0, 95.0):.2f}"  # Random new price
    ET.SubElement(item, "type").text = random.choice(types)
    ET.SubElement(item, "commodity_code").text = ''.join(random.choices(string.digits, k=9))  # Random 9-digit code
    ET.SubElement(item, "supplier").text = random.choice(suppliers)

    # Create a tree from the root element
    tree = ET.ElementTree(stock_item)
    
    # Write the XML to a file
    xml_file_path = os.path.join(output_dir, f"{unique_name}.xml")
    tree.write(xml_file_path, encoding='utf-8', xml_declaration=True)

print("400,000 XML files have been generated in the 'xml_files' directory.")
