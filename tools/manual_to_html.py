#!/usr/bin/env python3
"""Convert MANUAL.md to a self-contained, printable HTML page: manual_to_html.py MANUAL.md out.html"""
import sys, markdown

src, out = sys.argv[1], sys.argv[2]
body = markdown.markdown(open(src, encoding="utf-8").read(), extensions=["tables", "fenced_code"])
css = """
body { font-family: -apple-system, 'Segoe UI', Helvetica, Arial, sans-serif; max-width: 860px; margin: 40px auto;
       padding: 0 24px; color: #1d1d1f; line-height: 1.55; }
h1 { font-size: 30px; border-bottom: 3px solid #b5643a; padding-bottom: 8px; }
h2 { margin-top: 36px; color: #3a2416; } h3 { color: #5a3a24; }
table { border-collapse: collapse; width: 100%; margin: 14px 0; font-size: 14px; }
th, td { border: 1px solid #d9d4cc; padding: 7px 10px; text-align: left; vertical-align: top; }
th { background: #3a2416; color: #fff; } tr:nth-child(even) td { background: #faf6f1; }
code, pre { background: #f3efe9; border-radius: 4px; } pre { padding: 12px; overflow-x: auto; } code { padding: 1px 4px; }
@media print { body { margin: 0; } }
"""
html = f"""<!DOCTYPE html><html><head><meta charset="utf-8"><title>MacDanial Hills Mastering Compressor - User Manual</title>
<style>{css}</style></head><body>{body}</body></html>"""
open(out, "w", encoding="utf-8").write(html)
print("Wrote", out)
