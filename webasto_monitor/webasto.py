import tkinter as tk
from tkinter import ttk, font, messagebox
import serial
import serial.tools.list_ports
from threading import Thread, Event
import queue
import re
import os
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure

class WebastoMonitorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Webasto Monitor")
        self.log_window = None
        self.log_text = None

        # Параметры настроек по умолчанию
        self.settings_params = {
            'pump_size': "22",
            'heater_target': "195",
            'heater_min': "190",
            'heater_overheat': "210",
            'heater_warning': "200"
        }
        
        # Данные для графиков
        self.exhaust_temp_data = []
        self.fuel_hz_data = []
        self.max_data_points = 100
        
        # Загрузка шрифта DS-DIGI
        self.ds_digi_font = self.load_ds_digi_font()
        
        # Переменные для хранения данных
        self.data_vars = {
            'webasto_fail': tk.StringVar(value="N/A"),
            'ignit_fail': tk.StringVar(value="N/A"),
            'exhaust_temp': tk.StringVar(value="N/A"),
            'fan_speed': tk.StringVar(value="N/A"),
            'fuel_hz': tk.StringVar(value="N/A"),
            'fuel_need': tk.StringVar(value="N/A"),
            'glow_left': tk.StringVar(value="N/A"),
            'cycle_time': tk.StringVar(value="N/A"),
            'message': tk.StringVar(value="N/A"),
            'final_fuel': tk.StringVar(value="N/A"),
            'current_state': tk.StringVar(value="N/A")
        }
        
        # Флаг для остановки потока чтения
        self.stop_event = Event()
        self.serial_queue = queue.Queue()
        
        # Инициализация GUI
        self.setup_ui()

    def load_ds_digi_font(self):
        try:
            font_path = os.path.join(os.path.dirname(__file__), "DS-DIGI.ttf")
            if os.path.exists(font_path):
                return font.Font(family="DS-Digital", size=14, file=font_path)
            else:
                return font.Font(family="Courier", size=14, weight="bold")
        except:
            return font.Font(family="Courier", size=14, weight="bold")

    def setup_ui(self):
        style = ttk.Style()
        style.configure("TLabelframe", padding=5)
        style.configure("TLabelframe.Label", font=('Helvetica', 9, 'bold'))
        
      # Фрейм для настроек порта
        settings_frame = ttk.LabelFrame(self.root, text="COM Port Settings")
        settings_frame.grid(row=0, column=0, padx=10, pady=5, sticky="ew", columnspan=2)
        
        # Главный фрейм для верхней панели
        top_panel = ttk.Frame(self.root)
        top_panel.grid(row=0, column=0, padx=10, pady=5, sticky="ew", columnspan=2)
        
        # 1. Фрейм настроек COM-порта (слева)
        com_frame = ttk.LabelFrame(top_panel, text="COM Port Settings")
        com_frame.pack(side=tk.LEFT, padx=5, pady=0, fill=tk.Y)
        
        # Элементы COM-порта
        ttk.Label(com_frame, text="Port:").grid(row=0, column=0, padx=5, pady=5)
        self.port_combobox = ttk.Combobox(com_frame, values=self.get_available_ports(), width=12)
        self.port_combobox.grid(row=0, column=1, padx=5, pady=5)
        
        ttk.Label(com_frame, text="Baudrate:").grid(row=0, column=2, padx=5, pady=5)
        self.baud_combobox = ttk.Combobox(com_frame, values=[57600, 9600, 19200, 38400, 115200], width=8)
        self.baud_combobox.grid(row=0, column=3, padx=5, pady=5)
        self.baud_combobox.current(0)
        
        # Кнопка Connect в COM-фрейме
        self.connect_button = ttk.Button(com_frame, text="Connect", command=self.toggle_connection, width=13)
        self.connect_button.grid(row=0, column=4, padx=5, pady=5)

 
        # 2. Фрейм функций (справа) с увеличенной шириной
        func_frame = ttk.LabelFrame(top_panel, text="Functions")
        func_frame.pack(side=tk.RIGHT, padx=5, pady=0, fill=tk.Y)
        
        # Увеличиваем внутренние отступы фрейма
        func_frame.config(padding=(7, 5, 7, 5))  # (left, top, right, bottom)
        
        # Кнопки функций с увеличенной шириной
        self.clear_fail_button = ttk.Button(
            func_frame, 
            text="Clear Fail",  # Добавим пробел для лучшего отображения
            command=self.send_clear_fail,
            state='disabled',
            width=13  # Увеличиваем ширину кнопок
        )
        self.clear_fail_button.pack(side=tk.LEFT, padx=3, pady=2)

        self.fuel_pumping_button = ttk.Button(
            func_frame, 
            text="Fuel Pumping",  # Добавим пробел
            command=self.send_fuel_pumping,
            state='disabled',
            width=13  # Увеличиваем ширину кнопок
        )
        self.fuel_pumping_button.pack(side=tk.LEFT, padx=3, pady=2)

        # Add the new Log button (initially disabled)
        self.log_button = ttk.Button(
            func_frame,
            text="Log",
            command=self.open_log_window,
            state='disabled',
            width=13
        )
        self.log_button.pack(side=tk.LEFT, padx=3, pady=2)

        # Настройка минимальной ширины фрейма функций
        func_frame.update_idletasks()  # Обновляем геометрию
        func_frame.config(width=300)  # Устанавливаем желаемую ширину
        
        # Фрейм для отображения данных
        data_frame = ttk.LabelFrame(self.root, text="Webasto Data")
        data_frame.grid(row=1, column=0, padx=10, pady=5, sticky="nsew")
        
        # Создаем стиль для отображения данных
        style = ttk.Style()
        style.configure("Digital.TFrame", background="black")
        
        # Создаем метки и значения параметров
        self.create_digital_display(data_frame, "Webasto Fail:", "webasto_fail", 0)
        self.create_digital_display(data_frame, "Ign Fail #:", "ignit_fail", 1)
        self.create_digital_display(data_frame, "Exhaust Temp:", "exhaust_temp", 2)
        self.create_digital_display(data_frame, "Fan Speed %:", "fan_speed", 3)
        self.create_digital_display(data_frame, "Fuel HZ:", "fuel_hz", 4)
        self.create_digital_display(data_frame, "Fuel Need:", "fuel_need", 5)
        self.create_digital_display(data_frame, "Glow Left:", "glow_left", 6)
        self.create_digital_display(data_frame, "Cycle Time:", "cycle_time", 7)
        self.create_digital_display(data_frame, "Message:", "message", 8)
        self.create_digital_display(data_frame, "Final Fuel:", "final_fuel", 9)
        self.create_digital_display(data_frame, "State:", "current_state", 10)
        
        # Фрейм для кнопок управления
        buttons_frame = ttk.Frame(data_frame)
        buttons_frame.grid(row=11, column=0, columnspan=2, pady=(10, 5), sticky="ew")
        
        # Кнопки управления
        self.up_button = ttk.Button(
            buttons_frame, 
            text="Down", 
            command=self.send_up_command,
            state='disabled'
        )
        self.up_button.pack(side=tk.LEFT, padx=5, expand=True)
        
        self.enter_button = ttk.Button(
            buttons_frame, 
            text="Start/Stop", 
            command=self.send_enter_command,
            state='disabled'
        )
        self.enter_button.pack(side=tk.LEFT, padx=5, expand=True)
        
        self.down_button = ttk.Button(
            buttons_frame, 
            text="Up", 
            command=self.send_down_command,
            state='disabled'
        )
        self.down_button.pack(side=tk.LEFT, padx=5, expand=True)
        
        # Кнопка Settings
        self.settings_button = ttk.Button(
            data_frame, 
            text="Settings", 
            command=self.open_settings_window
        )
        self.settings_button.grid(row=12, column=0, columnspan=2, pady=(0, 10), sticky="ew", padx=10)
        
       # Фрейм для графиков
        graphs_frame = ttk.LabelFrame(self.root, text="Live Graphs")
        graphs_frame.grid(row=1, column=1, padx=10, pady=5, sticky="nsew", rowspan=2)
    
        # График температуры выхлопа
        self.setup_graph(graphs_frame, "Exhaust Temp", 0)
    
        # Новый график для Fuel HZ (вместо старого Fuel Pump Hz)
        self.setup_graph(graphs_frame, "Fuel HZ", 1)  # Изменено название
    
        # Настройка растягивания
        self.root.columnconfigure(0, weight=1)
        self.root.columnconfigure(1, weight=3)
        self.root.rowconfigure(1, weight=1)

    def send_clear_fail(self):
        """Отправка команды сброса ошибки Webasto (аналог длинного нажатия 2 сек)"""
        if hasattr(self, 'ser') and self.ser.is_open:
            try:
                self.ser.write(b'CF\n')
                self.log_message("Sent CLEAR_FAIL command")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to send CLEAR_FAIL command: {str(e)}")

    def send_fuel_pumping(self):
        """Отправка команды принудительной подкачки топлива (аналог удержания 10 сек)"""
        if hasattr(self, 'ser') and self.ser.is_open:
            try:
                self.ser.write(b'FP\n')
                self.log_message("Sent FUEL_PUMPING command")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to send FUEL_PUMPING command: {str(e)}")

                
    def setup_graph(self, parent, title, row):
        frame = ttk.Frame(parent)
        frame.grid(row=row, column=0, sticky="nsew", padx=5, pady=5)
        
        fig = Figure(figsize=(5, 2), dpi=100)
        ax = fig.add_subplot(111)
        ax.set_title(title)
        ax.set_facecolor('black')
        fig.patch.set_facecolor('black')
        ax.tick_params(colors='white')
        ax.xaxis.label.set_color('white')
        ax.yaxis.label.set_color('white')
        ax.title.set_color('white')
        
        if title == "Exhaust Temp":
            self.exhaust_temp_line, = ax.plot([], [], 'g-')
            ax.set_ylim(0, 250)
            self.exhaust_temp_canvas = FigureCanvasTkAgg(fig, master=frame)
            self.exhaust_temp_canvas.draw()
            self.exhaust_temp_canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
            self.exhaust_temp_ax = ax
        else:  # График Fuel HZ
            self.fuel_hz_line, = ax.plot([], [], 'y-')
            ax.set_ylim(0, 10)  # Можно настроить под ваш диапазон Hz
            self.fuel_hz_canvas = FigureCanvasTkAgg(fig, master=frame)
            self.fuel_hz_canvas.draw()
            self.fuel_hz_canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
            self.fuel_hz_ax = ax

    def update_graphs(self):
        # Обновляем график температуры выхлопа
        if len(self.exhaust_temp_data) > 0:
            x_data = range(len(self.exhaust_temp_data))
            self.exhaust_temp_line.set_data(x_data, self.exhaust_temp_data)
            self.exhaust_temp_ax.relim()
            self.exhaust_temp_ax.autoscale_view(True, True, True)
            self.exhaust_temp_canvas.draw()
        
        # Обновляем график частоты топливного насоса
        if len(self.fuel_hz_data) > 0:
            x_data = range(len(self.fuel_hz_data))
            self.fuel_hz_line.set_data(x_data, self.fuel_hz_data)
            self.fuel_hz_ax.relim()
            self.fuel_hz_ax.autoscale_view(True, True, True)
            self.fuel_hz_canvas.draw()
        
        # Планируем следующее обновление
        if not self.stop_event.is_set():
            self.root.after(500, self.update_graphs)

    def create_digital_display(self, frame, label_text, var_name, row):
        row_frame = ttk.Frame(frame)
        row_frame.grid(row=row, column=0, sticky="ew", padx=5, pady=2)
        
        label = ttk.Label(row_frame, text=label_text, width=15, anchor="e")
        label.grid(row=0, column=0, padx=5, sticky="e")
        
        value_frame = ttk.Frame(row_frame, style="Digital.TFrame")
        value_frame.grid(row=0, column=1, sticky="ew")
        
        value_label = tk.Label(
            value_frame, 
            textvariable=self.data_vars[var_name],
            font=self.ds_digi_font,
            fg="#00FF00",
            bg="black",
            padx=5,
            anchor="w"
        )
        value_label.pack(fill=tk.X, expand=True)
        
        row_frame.columnconfigure(1, weight=1)

    def open_settings_window(self):
        self.settings_window = tk.Toplevel(self.root)
        self.settings_window.title("Settings")
        self.settings_window.geometry("400x450")  # Увеличили высоту для предупреждения
        
        # Фрейм для параметров настроек
        settings_frame = ttk.LabelFrame(self.settings_window, text="Configuration Parameters")
        settings_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # Создаем элементы управления для каждого параметра с подсказками
        self.settings_entries = {}
        params = [
            ("Pump Size (ml):", "pump_size", "Размер насоса (любое число, по умолчанию 22). Можно корректировать по мере износа. Больше число = меньше производительность."),
            ("Heater Target Temp (C):", "heater_target", "Целевая температура для нагревателя, градусы Цельсия. При установке больше 250, установятся все значения настроек по-умочанию после перезагрузки"),
            ("Heater Min Temp (C):", "heater_min", "Минимальная температура для нагревателя, градусы Цельсия"),
            ("Heater Overheat Temp (C):", "heater_overheat", "Температура перегрева нагревателя, градусы Цельсия"),
            ("Heater Warning Temp (C):", "heater_warning", "Температура предупреждения для нагревателя, градусы Цельсия")
        ]
        
        for i, (label, param, tooltip) in enumerate(params):
            frame = ttk.Frame(settings_frame)
            frame.grid(row=i, column=0, sticky="ew", padx=5, pady=2)
            
            # Создаем Label с возможностью добавления подсказки
            lbl = ttk.Label(frame, text=label, width=25, anchor="e")
            lbl.grid(row=0, column=0, padx=5, sticky="e")
            
            # Создаем Entry с возможностью добавления подсказки
            entry = ttk.Entry(frame)
            entry.insert(0, self.settings_params.get(param, ""))
            entry.grid(row=0, column=1, sticky="ew")
            
            # Добавляем подсказки к Label и Entry
            self.create_tooltip(lbl, tooltip)
            self.create_tooltip(entry, tooltip)
            
            self.settings_entries[param] = entry
            frame.columnconfigure(1, weight=1)
        
        # Фрейм для предупреждения
        warning_frame = ttk.Frame(self.settings_window)
        warning_frame.pack(fill=tk.X, padx=10, pady=(0, 10))
        
        warning_label = ttk.Label(
            warning_frame, 
            text="Остальные настройки лучше править в коде, сильно зависят друг от друга!!! Считывать и применять настройки когда нагреватель OFF", 
            foreground="red",
            wraplength=380,
            justify=tk.CENTER
        )
        warning_label.pack(fill=tk.X)
        
        # Кнопки управления
        buttons_frame = ttk.Frame(self.settings_window)
        buttons_frame.pack(fill=tk.X, padx=10, pady=10)
        
        ttk.Button(
            buttons_frame, 
            text="Read Settings", 
            command=self.read_settings
        ).pack(side=tk.LEFT, padx=5)
        
        ttk.Button(
            buttons_frame, 
            text="Save Settings", 
            command=self.save_settings
        ).pack(side=tk.RIGHT, padx=5)
        
        ttk.Button(
            buttons_frame, 
            text="Cancel", 
            command=self.settings_window.destroy
        ).pack(side=tk.RIGHT, padx=5)

    def create_tooltip(self, widget, text):
        """Создает всплывающую подсказку для виджета"""
        tooltip = tk.Toplevel(self.root)
        tooltip.withdraw()
        tooltip.overrideredirect(True)
        
        label = ttk.Label(tooltip, text=text, background="#ffffe0", relief="solid", borderwidth=1, 
                         padding=(5, 3, 5, 3), wraplength=300)
        label.pack()
        
        def enter(event):
            x = widget.winfo_rootx() + widget.winfo_width() + 5
            y = widget.winfo_rooty() + (widget.winfo_height() // 2)
            tooltip.geometry(f"+{x}+{y}")
            tooltip.deiconify()
        
        def leave(event):
            tooltip.withdraw()
        
        widget.bind("<Enter>", enter)
        widget.bind("<Leave>", leave)
        tooltip.bind("<Leave>", leave)

    def read_settings(self):
        if hasattr(self, 'ser') and self.ser.is_open:
            self.ser.write(b'GET_SETTINGS\n')
            self.log_message("Sent GET_SETINGS command")
        else:
            messagebox.showerror("Error", "Not connected to device!")

    def save_settings(self):
        if not hasattr(self, 'ser') or not self.ser.is_open:
            messagebox.showerror("Error", "Not connected to device!")
            return
            
        settings_command = "SET:"
        settings_command += ",".join(
            f"{k}={v.get()}" for k, v in self.settings_entries.items()
        )
        settings_command += "\n"
        
        try:
            self.ser.write(settings_command.encode())
            self.log_message(f"Sent settings: {settings_command.strip()}")
            messagebox.showinfo("Success", "Settings sent to device")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to send settings: {str(e)}")

    def send_up_command(self):
        """Отправка команды увеличения уровня мощности"""
        if hasattr(self, 'ser') and self.ser.is_open:
            try:
                self.ser.write(b'UP\n')
                self.log_message("Sent UP command")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to send UP command: {str(e)}")

    def send_down_command(self):
        """Отправка команды уменьшения уровня мощности"""
        if hasattr(self, 'ser') and self.ser.is_open:
            try:
                self.ser.write(b'DOWN\n')
                self.log_message("Sent DOWN command")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to send DOWN command: {str(e)}")

    def send_enter_command(self):
        """Отправка команды включения/выключения нагрева"""
        if hasattr(self, 'ser') and self.ser.is_open:
            try:
                self.ser.write(b'ENTER\n')
                self.log_message("Sent ENTER command")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to send ENTER command: {str(e)}")

    def process_serial_queue(self):
        try:
            while not self.serial_queue.empty():
                line = self.serial_queue.get_nowait()
                
                if line.startswith("CURRENT_SETTINGS:"):
                    self.parse_settings(line[17:])
                elif line.startswith("SETTINGS_OK"):
                    self.log_message("Device confirmed settings update")
                elif line.startswith("SETTINGS_ERROR"):
                    self.log_message("Device reported settings error")
                elif "St:" in line:
                    # Обработка состояния системы с преобразованием в текстовый формат
                    state_part = line.split("St: ")[1].split()[0]
                    state_mapping = {'0': 'FULL', '1': 'MID', '2': 'LOW'}
                    display_state = state_mapping.get(state_part, state_part)
                    self.data_vars['current_state'].set(display_state)
                else:
                    # Парсим обычные данные Webasto
                    self.parse_data(line)
                
        except queue.Empty:
            pass
        
        if not self.stop_event.is_set():
            self.root.after(100, self.process_serial_queue)

    def parse_data(self, line):
        patterns = {
            'webasto_fail': r'F: (\S+)',
            'ignit_fail': r'IgnF#: (\S+)',
            'exhaust_temp': r'ETmp: (\S+)',
            'fan_speed': r'Fan%: (\S+)',
            'fuel_hz': r'FHZ (\S+)',  # Это значение теперь будет использоваться для графика
            'fuel_need': r'FN: (\S+)',
            'glow_left': r'Gl: (\S+)',
            'cycle_time': r'CyTim: (\S+)',
            'message': r'I: (\S+)',
            'final_fuel': r'FinalFuel: (\S+)'
        }
        
        for var_name, pattern in patterns.items():
            match = re.search(pattern, line)
            if match:
                value = match.group(1)
                
                # Преобразование состояния (0 → FULL, 1 → MID, 2 → LOW)
                if var_name == 'current_state':
                    state_mapping = {'0': 'FULL', '1': 'MID', '2': 'LOW'}
                    value = state_mapping.get(value, value)
                
                self.data_vars[var_name].set(value)

                try:
                    if var_name == "exhaust_temp":
                        temp_str = value[:4]  # Берем первые 4 символа
                        temp = float(temp_str)
                        self.exhaust_temp_data.append(temp)
                        if len(self.exhaust_temp_data) > self.max_data_points:
                            self.exhaust_temp_data.pop(0)
                    
                    elif var_name == "fuel_hz":  # Теперь обновляем график Fuel HZ
                        hz = float(value)
                        self.fuel_hz_data.append(hz)  # Используем данные из "Fuel HZ:"
                        if len(self.fuel_hz_data) > self.max_data_points:
                            self.fuel_hz_data.pop(0)
                            
                except ValueError:
                    pass

    def parse_settings(self, data):
        try:
            params = data.split(',')
            for param in params:
                if '=' in param:
                    key, value = param.split('=')
                    if key in self.settings_entries:
                        self.settings_entries[key].delete(0, tk.END)
                        self.settings_entries[key].insert(0, value)
                        self.settings_params[key] = value
            self.log_message("Updated settings from device")
        except Exception as e:
            self.log_message(f"Error parsing settings: {str(e)}")

    def get_available_ports(self):
        ports = serial.tools.list_ports.comports()
        return [port.device for port in ports]

    def toggle_connection(self):
        if hasattr(self, 'ser') and self.ser.is_open:
            self.disconnect_serial()
            self.connect_button.config(text="Connect")
            # Отключаем все кнопки управления при разрыве соединения
            self.up_button.config(state='disabled')
            self.down_button.config(state='disabled')
            self.enter_button.config(state='disabled')
            self.clear_fail_button.config(state='disabled')
            self.fuel_pumping_button.config(state='disabled')
            self.log_button.config(state='disabled')  # Disable Log button too
        else:
            if self.connect_serial():
                self.connect_button.config(text="Disconnect")
                self.root.after(500, self.update_graphs)
                # Включаем кнопки управления при успешном подключении
                self.up_button.config(state='normal')
                self.down_button.config(state='normal')
                self.enter_button.config(state='normal')
                self.clear_fail_button.config(state='normal')
                self.fuel_pumping_button.config(state='normal')
                self.log_button.config(state='normal')  # Enable Log button

    def connect_serial(self):
        port = self.port_combobox.get()
        baudrate = int(self.baud_combobox.get())
        
        try:
            self.ser = serial.Serial(port, baudrate, timeout=1)
            self.stop_event.clear()
            self.read_thread = Thread(target=self.read_from_port, daemon=True)
            self.read_thread.start()
            self.root.after(100, self.process_serial_queue)
            return True
        except Exception as e:
            messagebox.showerror("Error", f"Connection error: {str(e)}")
            return False

    def disconnect_serial(self):
        self.stop_event.set()
        if hasattr(self, 'ser') and self.ser.is_open:
            self.ser.close()

    def read_from_port(self):
        while not self.stop_event.is_set():
            if hasattr(self, 'ser') and self.ser.is_open:
                try:
                    line = self.ser.readline().decode('utf-8', errors='replace').strip()
                    if line:
                        self.serial_queue.put(line)
                        self.log_message(line)  # Log all incoming data
                except Exception as e:
                    error_msg = f"Read error: {str(e)}"
                    self.serial_queue.put(error_msg)
                    self.log_message(error_msg)
                    break

    def log_message(self, message):
        """Enhanced log_message that writes to both console and log window"""
        print(message)  # Console output
        
        if self.log_text and self.log_window and self.log_window.winfo_exists():
            self.log_text.config(state='normal')
            self.log_text.insert(tk.END, message + "\n")
            self.log_text.see(tk.END)
            self.log_text.config(state='disabled')

    def on_closing(self):
        self.disconnect_serial()
        self.root.destroy()

    def open_log_window(self):
        if self.log_window is None or not self.log_window.winfo_exists():
            self.log_window = tk.Toplevel(self.root)
            self.log_window.title("Serial Port Log")
            self.log_window.geometry(f"{self.root.winfo_width()}x{self.root.winfo_height()}")
            
            # Create main frame
            main_frame = ttk.Frame(self.log_window)
            main_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
            
            # Create text widget for log display
            self.log_text = tk.Text(
                main_frame,
                wrap=tk.WORD,
                state='disabled',
                bg='black',
                fg='white',
                font=('Courier', 10)
            )
            self.log_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
            
            # Create scrollbar
            scrollbar = ttk.Scrollbar(main_frame, command=self.log_text.yview)
            scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
            self.log_text.config(yscrollcommand=scrollbar.set)
            
            # Create bottom frame for entry and buttons
            bottom_frame = ttk.Frame(main_frame)
            bottom_frame.pack(fill=tk.X, padx=5, pady=5)
            
            # Entry for sending commands
            self.log_entry = ttk.Entry(bottom_frame)
            self.log_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)
            self.log_entry.bind('<Return>', self.send_serial_command)
            
            # Send button
            send_button = ttk.Button(
                bottom_frame,
                text="Send",
                command=self.send_serial_command
            )
            send_button.pack(side=tk.LEFT, padx=5)
            
            # Clear button
            clear_button = ttk.Button(
                bottom_frame,
                text="Clear",
                command=self.clear_log
            )
            clear_button.pack(side=tk.LEFT, padx=5)
            
            # Configure window close behavior
            self.log_window.protocol("WM_DELETE_WINDOW", self.close_log_window)
        else:
            self.log_window.lift()

    def close_log_window(self):
        if self.log_window:
            self.log_window.destroy()
            self.log_window = None

    def clear_log(self):
        if self.log_text:
            self.log_text.config(state='normal')
            self.log_text.delete(1.0, tk.END)
            self.log_text.config(state='disabled')

    def send_serial_command(self, event=None):
        if hasattr(self, 'ser') and self.ser.is_open and self.log_entry:
            command = self.log_entry.get()
            if command:
                try:
                    self.ser.write((command + '\n').encode())
                    self.log_message(f">>> {command}")
                    self.log_entry.delete(0, tk.END)
                except Exception as e:
                    self.log_message(f"Error sending command: {str(e)}")
    
    def log_message(self, message):
        """Enhanced log_message that also writes to the log window"""
        print(message)  # Console output
        
        if self.log_text and self.log_window and self.log_window.winfo_exists():
            self.log_text.config(state='normal')
            self.log_text.insert(tk.END, message + "\n")
            self.log_text.see(tk.END)
            self.log_text.config(state='disabled')

if __name__ == "__main__":
    root = tk.Tk()
    app = WebastoMonitorApp(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()