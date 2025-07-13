#!/usr/bin/env python3
"""
Fix HTML file references to use simple filenames instead of hashed asset names.
"""

import os
import re

def fix_html_references():
    """Fix the HTML file to reference index.css and index.js instead of hashed names."""
    
    html_file = "data_clean/index.html"
    
    if not os.path.exists(html_file):
        print(f"❌ HTML file not found: {html_file}")
        return False
    
    print(f"🔧 Fixing HTML references in {html_file}")
    
    # Read the HTML file
    with open(html_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    print(f"📄 Original content:")
    print(content)
    
    # Replace asset references with simple names
    # Replace CSS reference
    content = re.sub(r'href="/assets/[^"]*\.css"', 'href="/index.css"', content)
    
    # Replace JS reference  
    content = re.sub(r'src="/assets/[^"]*\.js"', 'src="/index.js"', content)
    
    print(f"\n📄 Fixed content:")
    print(content)
    
    # Write the fixed HTML file
    with open(html_file, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print(f"✅ HTML references fixed successfully")
    return True

if __name__ == "__main__":
    if fix_html_references():
        print("\n🎉 HTML file fixed!")
    else:
        print("\n❌ Failed to fix HTML file")
