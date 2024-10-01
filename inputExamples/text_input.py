import tkinter as tk
import socket
from functools import partial

# muji_moe default port 12888
client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
client_socket.settimeout(1.0)

def send_text(input):
    input_str = input.get("1.0", tk.END).strip('\n')
    if len(input_str) == 0:
        return
    
    client_socket.sendto(input_str.encode(), ('localhost', 12888))

    input.delete("1.0", tk.END)

root = tk.Tk(screenName=None, baseName=None, className='muji_moe', useTk=1)
root.resizable(False,False)

# ttk.Label(frm, text="Hello World!").grid(column=0, row=0)
input = tk.Text(root, width=50, height=10)
input.pack()

tk.Button(root, text="Send", command=partial(send_text, input)).pack(side=tk.LEFT, fill=tk.BOTH, expand=1)
# ttk.Button(frm, text="Quit", command=root.destroy).grid(column=1, row=0)

root.mainloop()