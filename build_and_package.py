import os
import sys
import json
import shutil
import subprocess
import threading
import tkinter as tk
from tkinter import messagebox, filedialog, ttk

class BoboPackagerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("BOBO 2.2 Firmware Builder & Packager")
        self.root.geometry("520x340")
        self.root.resizable(False, False)
        
        # Style
        self.style = ttk.Style()
        self.style.theme_use('clam')
        
        # Color Theme (Orange and Dark Gray match BOBO UI)
        self.bg_color = "#161616"
        self.fg_color = "#efefef"
        self.accent_color = "#ff6b1a"
        self.btn_color = "#252525"
        
        self.root.configure(bg=self.bg_color)
        
        # Header Label
        header_frame = tk.Frame(self.root, bg=self.bg_color)
        header_frame.pack(fill=tk.X, pady=15, padx=20)
        
        title_lbl = tk.Label(header_frame, text="BOBO 2.2 Build System", font=("Space Grotesk", 16, "bold"), bg=self.bg_color, fg=self.accent_color)
        title_lbl.pack(anchor=tk.W)
        
        desc_lbl = tk.Label(header_frame, text="Compile firmware and export web installer assets for your OLED variant.", font=("Space Grotesk", 9), bg=self.bg_color, fg="#a8a8a8")
        desc_lbl.pack(anchor=tk.W, pady=2)
        
        # Option Frame
        opt_frame = tk.LabelFrame(self.root, text=" 1. Select Display Type ", font=("Space Grotesk", 10, "bold"), bg=self.bg_color, fg=self.accent_color, bd=1, relief=tk.SOLID, padx=15, pady=10)
        opt_frame.pack(fill=tk.X, padx=20, pady=10)
        
        self.disp_var = tk.StringVar(value="ssd1306")
        
        r_ssd = tk.Radiobutton(opt_frame, text="SSD1306 OLED (Standard 0.96\" screen)", variable=self.disp_var, value="ssd1306", bg=self.bg_color, fg=self.fg_color, selectcolor=self.bg_color, font=("Space Grotesk", 10), activebackground=self.bg_color, activeforeground=self.fg_color)
        r_ssd.pack(anchor=tk.W, pady=4)
        
        r_sh = tk.Radiobutton(opt_frame, text="SH110X OLED (Alternative 1.3\" screen)", variable=self.disp_var, value="sh110x", bg=self.bg_color, fg=self.fg_color, selectcolor=self.bg_color, font=("Space Grotesk", 10), activebackground=self.bg_color, activeforeground=self.fg_color)
        r_sh.pack(anchor=tk.W, pady=4)
        
        # Action Frame
        act_frame = tk.Frame(self.root, bg=self.bg_color)
        act_frame.pack(fill=tk.X, padx=20, pady=15)
        
        self.build_btn = tk.Button(act_frame, text="Build & Package Firmware", command=self.start_build_thread, font=("Space Grotesk", 10, "bold"), bg=self.accent_color, fg="#0c0c0c", activebackground="#ff8c42", bd=0, cursor="hand2", padx=20, pady=8)
        self.build_btn.pack(side=tk.LEFT)
        
        self.status_lbl = tk.Label(act_frame, text="Ready", font=("Space Grotesk", 10, "italic"), bg=self.bg_color, fg="#a8a8a8")
        self.status_lbl.pack(side=tk.LEFT, padx=15)

    def log_status(self, text, color="#a8a8a8"):
        self.status_lbl.config(text=text, fg=color)
        self.root.update_idletasks()

    def start_build_thread(self):
        self.build_btn.config(state=tk.DISABLED)
        thread = threading.Thread(target=self.run_build_and_package)
        thread.start()

    def run_build_and_package(self):
        display_type = self.disp_var.get()
        env_name = display_type
        display_upper = display_type.upper()
        
        self.log_status(f"Compiling for {display_upper}...", self.accent_color)
        
        # Check platformio command
        pio_cmd = "pio"
        # On Windows, try to find in standard path if pio is not on PATH
        if shutil.which(pio_cmd) is None:
            home = os.path.expanduser("~")
            possible_pio_paths = [
                os.path.join(home, ".platformio", "penv", "Scripts", "pio.exe"),
                os.path.join(home, ".platformio", "penv", "Scripts", "pio"),
            ]
            for p in possible_pio_paths:
                if os.path.exists(p):
                    pio_cmd = p
                    break
        
        try:
            # Run compiler
            res = subprocess.run([pio_cmd, "run", "-e", env_name], capture_output=True, text=True, check=True)
            print(res.stdout)
        except subprocess.CalledProcessError as e:
            print(e.stdout)
            print(e.stderr)
            self.log_status("Compilation failed!", "#ff3b3b")
            messagebox.showerror("Error", f"PlatformIO compilation failed!\n\n{e.stderr or e.stdout[:300]}")
            self.build_btn.config(state=tk.NORMAL)
            return
        except FileNotFoundError:
            self.log_status("PlatformIO not found!", "#ff3b3b")
            messagebox.showerror("Error", "Could not find 'pio' executable on your PATH or in ~/.platformio/penv/Scripts/.\n\nPlease install PlatformIO CLI or add it to system environment variables.")
            self.build_btn.config(state=tk.NORMAL)
            return

        self.log_status("Compilation successful!", "#22c55e")
        
        # Ask folder to copy files
        dest_dir = filedialog.askdirectory(title=f"Select Destination Folder for {display_upper} Files")
        if not dest_dir:
            self.log_status("Packaging cancelled", "#fbbf24")
            self.build_btn.config(state=tk.NORMAL)
            return

        # Prepare paths
        build_dir = os.path.join(".pio", "build", env_name)
        fw_src = os.path.join(build_dir, "firmware.bin")
        bl_src = os.path.join(build_dir, "bootloader.bin")
        pt_src = os.path.join(build_dir, "partitions.bin")
        
        if not (os.path.exists(fw_src) and os.path.exists(bl_src) and os.path.exists(pt_src)):
            self.log_status("Build artifacts missing!", "#ff3b3b")
            messagebox.showerror("Error", f"Could not find build files in {build_dir}. Ensure compilation compiled successfully.")
            self.build_btn.config(state=tk.NORMAL)
            return

        # Define output names
        fw_name = f"firmware_{display_type}.bin"
        bl_name = "bootloader.bin"
        pt_name = "partitions.bin"
        
        # Copy files
        try:
            shutil.copy2(fw_src, os.path.join(dest_dir, fw_name))
            shutil.copy2(bl_src, os.path.join(dest_dir, bl_name))
            shutil.copy2(pt_src, os.path.join(dest_dir, pt_name))
            
            # Generate manifest file
            manifest_data = {
                "name": f"BOBO 2.2 Personality Engine ({display_upper})",
                "version": "2.2.0",
                "home_assistant_domain": "esphome",
                "new_install_prompt_erase": True,
                "builds": [
                    {
                        "chipFamily": "ESP32-C3",
                        "improv": False,
                        "parts": [
                            { "path": bl_name, "offset": 0 },
                            { "path": pt_name, "offset": 32768 },
                            { "path": fw_name, "offset": 65536 }
                        ]
                    }
                ]
            }
            
            manifest_file_name = f"manifest_{display_type}.json"
            manifest_path = os.path.join(dest_dir, manifest_file_name)
            with open(manifest_path, 'w') as f:
                json.dump(manifest_data, f, indent=2)
                
            self.log_status("Export completed!", "#22c55e")
            messagebox.showinfo("Success", f"Build succeeded and files packaged successfully to:\n\n{dest_dir}\n\nGenerated files:\n- {fw_name}\n- {bl_name}\n- {pt_name}\n- {manifest_file_name}")
            
        except Exception as ex:
            self.log_status("Export failed!", "#ff3b3b")
            messagebox.showerror("Error", f"Failed to package files: {str(ex)}")
            
        self.build_btn.config(state=tk.NORMAL)

if __name__ == "__main__":
    root = tk.Tk()
    app = BoboPackagerApp(root)
    root.mainloop()
