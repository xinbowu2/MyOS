template = """void interrupt_handler_{0}(void) {{EXCEPTION("      {0}\\n");}}"""

for i in range(19,256):
    print(template.format(i))

for i in range(19, 256):
    print("SET_INTERRUPT_GATE(idt[{0}], interrupt_handler_{0});".format(i))
