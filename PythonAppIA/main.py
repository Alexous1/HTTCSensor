import ctypes
import dearpygui.dearpygui as dpg
import threading
import serial
import time
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib.backends.backend_agg import FigureCanvasAgg
import matplotlib

matplotlib.use("agg")

# ===========================================
# CONFIGURATION
# ===========================================
SERIAL_PORT = "COM3"
BAUD_RATE = 115200

ctypes.windll.shcore.SetProcessDpiAwareness(2)
dpg.create_context()
dpg.create_viewport(title="Capteurs TVOC/CO2", width=1920, height=1080)

# ===========================================
# THÈME (NeoDark intégré)
# ===========================================
with dpg.theme() as THEME:
    with dpg.font_registry():
        fontsize = 40
        default_font = dpg.add_font("ressources/consola.ttf", fontsize)
    dpg.bind_font(default_font)
    dpg.set_global_font_scale(0.5)
    with dpg.theme_component(dpg.mvAll):
        dpg.add_theme_color(dpg.mvThemeCol_WindowBg, (40, 40, 50, 255))
        dpg.add_theme_color(dpg.mvThemeCol_ButtonHovered, (30, 60, 80, 255))
        dpg.add_theme_color(dpg.mvThemeCol_Border, (110, 110, 128, 128))
        dpg.add_theme_color(dpg.mvThemeCol_TitleBg, (85, 85, 85, 255))
        dpg.add_theme_style(dpg.mvStyleVar_FrameRounding, 5)
        dpg.add_theme_style(dpg.mvStyleVar_WindowRounding, 5)
        dpg.add_theme_style(dpg.mvStyleVar_ItemSpacing, 10, 5)

dpg.bind_theme(THEME)

# ===========================================
# VARIABLES GLOBALES
# ===========================================
start_time = time.time()


# globals variables
humidity_data = []
temperature_data = []
tvoc_data = []
co2_data = []
time_data = []
time_data1 = []

# data just for the plot
humidity_dataPlot = []
temperature_dataPlot = []
tvoc_dataPlot = []
co2_dataPlot = []
time_dataPlot = []
time_dataPlot1 = []


lock = threading.Lock()



# ===========================================
# FENÊTRE PRINCIPALE
# ===========================================
class MainWin:
    def __init__(self):
        self.winID = "main_win"
        with dpg.window(tag=self.winID) as win_main:
            with dpg.menu_bar():
                with dpg.menu(label="Tools"):
                    dpg.add_menu_item(label="Show Debug", callback=lambda: dpg.show_tool(dpg.mvTool_Debug))
                    dpg.add_menu_item(label="Show Metrics", callback=lambda: dpg.show_tool(dpg.mvTool_Metrics))
                    dpg.add_menu_item(label="Toggle Fullscreen", callback=lambda: dpg.toggle_viewport_fullscreen())

            with dpg.theme() as mainwin_theme:
                with dpg.theme_component(dpg.mvAll):
                    dpg.add_theme_color(dpg.mvThemeCol_WindowBg, (20, 20, 25, 255))
            dpg.bind_item_theme(win_main, mainwin_theme)


main_win = MainWin()


# ===========================================
# CLASSE POUR LE GRAPHIQUE ET TABLES
# ===========================================
class PlotWin:
    def __init__(self):
        self.winID = "plot_win"
        self.winID2 = "plot_win2"
        self.winID3 = "plot_node"
        self.plotID = "plot"
        self.plotID2 = "plot2"
        self.plotID3 = "plot3"
        self.datawin1ID = "data_win_tvoc"
        self.datawin2ID = "data_win_co2"
        self.table_tvoc = "tvoc_table"
        self.table_co2 = "co2_table"
        self.datawin3ID = "data_win_temp"
        self.datawin4ID = "data_win_hum"
        self.table_temp = "temp_table"
        self.table_hum = "hum_table"
        self.inputText = "input_text"
        self.serialPort = "serialPort"
        self.datawin5ID = "data_serial_port"
        self.X = ""
        self.Y = ""
        self.next_row_id = 0

        # Fenêtre graphique
        with dpg.window(label="Graphique Capteurs", pos=(25, 50), width=1200, height=900, tag=self.winID):
            with dpg.group(horizontal=True):
                dpg.add_button(label="Exporter Matplotlib", callback=self.plot_canvas)

            with dpg.plot(label="Capteurs", height=-1, width=-1, tag=self.plotID):
                dpg.add_plot_legend()
                dpg.add_plot_axis(dpg.mvXAxis, label="Temps (s)", tag="x_axis0")

                with dpg.plot_axis(dpg.mvYAxis, label="TVOC (ppb)", tag="y_axis00"):
                    dpg.add_line_series([], [], label="TVOC (ppb)", tag="tvoc_series")

                with dpg.plot_axis(dpg.mvYAxis, label="eCO2 (ppm)", tag="y_axis01"):
                    dpg.add_line_series([], [], label="eCO2 (ppm)", tag="co2_series")

        with dpg.window(label="Graphique Capteurs2", pos=(25, 100), width=1200, height=900, tag=self.winID2):

            with dpg.plot(label="Capteurs", height=-1, width=-1, tag=self.plotID2):
                dpg.add_plot_legend()
                dpg.add_plot_axis(dpg.mvXAxis, label="Temps (s)", tag="x_axis1")
                with dpg.plot_axis(dpg.mvYAxis, label="Température °C", tag="y_axis10"):
                    dpg.add_line_series([], [], label="Température °C", tag="Température_series")

                with dpg.plot_axis(dpg.mvYAxis, label="Humidité %", tag="y_axis11"):
                    dpg.add_line_series([], [], label="Humidité %", tag="Humidité_series")

        # plot create with node
        with dpg.window(label="Node plot", pos=(25, 100), width=1200, height=900, tag=self.winID3, show=False):

            with dpg.plot(label="Capteurs", height=-1, width=-1, tag=self.plotID3):
                dpg.add_plot_legend()
                
                
                dpg.add_drag_line(label="Start", color=[8, 204, 23, 255], vertical=True, default_value=0, tag="dline2")
                dpg.add_drag_line(label="Finish", color=[222, 20, 20, 255], vertical=True, default_value=1, tag="dline3")
                dpg.add_drag_line(label="dline1", color=[255, 255, 0, 255], vertical=False, default_value=1, tag="dline1")
                dpg.add_plot_axis(dpg.mvXAxis, label="Temps (s)", tag="x_axis2")
                with dpg.plot_axis(dpg.mvYAxis, label="Valeur", tag="y_axis2"):
                    dpg.add_line_series([], [], label="", tag="node_first_serie")
                with dpg.plot_axis(dpg.mvYAxis, label="Valeur", tag="y_axis3"):    
                    dpg.add_line_series([], [], label="", tag="second")

        # Fenêtre Données TVOC
        with dpg.window(label="Données TVOC", tag=self.datawin1ID, pos=(1250, 50), width=300, height=900, show=True):
            dpg.add_button(label="delete", callback= lambda : self.resetValue(self.table_tvoc))
            with dpg.table(header_row=True, tag=self.table_tvoc):
                dpg.add_table_column(label="Temps (s)")
                dpg.add_table_column(label="TVOC (ppb)")

        # Fenêtre Données CO2       
        with dpg.window(label="Données CO2", tag=self.datawin2ID, pos=(1575, 50), width=300, height=900, show=True):
            dpg.add_button(label="delete", callback= lambda : self.resetValue(self.table_co2))
            with dpg.table(header_row=True, tag=self.table_co2):               
                dpg.add_table_column(label="Temps (s)")
                dpg.add_table_column(label="CO2 (ppm)")

        # Fenêtre Données HUM  
        with dpg.window(label="Données Humidity", tag=self.datawin4ID, pos=(1575, 50), width=300, height=900, show=True):
            dpg.add_button(label="delete", callback= lambda : self.resetValue(self.table_hum))
            with dpg.table(header_row=True, tag=self.table_hum):               
                dpg.add_table_column(label="Temps (s)")
                dpg.add_table_column(label="Humidity (%)")

        # Fenêtre Données HUM  
        with dpg.window(label="Données Température", tag=self.datawin3ID, pos=(1575, 50), width=300, height=900, show=True):
            dpg.add_button(label="delete", callback= lambda : self.resetValue(self.table_temp))
            with dpg.table(header_row=True, tag=self.table_temp):               
                dpg.add_table_column(label="Temps (s)")
                dpg.add_table_column(label="Température (%)")


        with dpg.window(label="Serial Port", tag=self.datawin5ID, pos=(1275, 50), width=500, height=300, show=True):
            dpg.add_input_text(label="Command", default_value="", tag=self.inputText, width=320)
            dpg.add_text("Output will appear here", tag=self.serialPort)


        # thème / callback
        with dpg.theme() as table_theme:
            with dpg.theme_component(dpg.mvTable):
                dpg.add_theme_color(dpg.mvThemeCol_HeaderActive, (0,0,0,0), category=dpg.mvThemeCat_Core)
                dpg.add_theme_color(dpg.mvThemeCol_Header, (0,0,0,0), category=dpg.mvThemeCat_Core)


        with dpg.window(label="Test", tag="efzdf", pos=(1275, 50), width=500, height=300, show=True):
            dpg.add_button(label="actualise", callback= lambda : self.getData())
            with dpg.table(header_row=True, tag="dataChoosen", clipper=True):
                # [début, fin, max, min, moyenne, deltaS, deltaM]
                dpg.add_table_column(label="Wv")
                dpg.add_table_column(label="Fv")
                dpg.add_table_column(label="Lv")
                dpg.add_table_column(label="Max")
                dpg.add_table_column(label="Min")
                dpg.add_table_column(label="moyenne")
                dpg.add_table_column(label="deltaS")
                dpg.add_table_column(label="deltaM")
                dpg.add_table_column(label="del")

        # callback runs when user attempts to connect attributes
        def link_callback(sender, app_data):
            # app_data -> (link_id1, link_id2)
            dpg.add_node_link(app_data[0], app_data[1], parent=sender)

            self.X = dpg.get_value("X")
            self.Y = dpg.get_value("Y")

            dpg.set_value("fonction_output", f"{self.X}=f({self.Y})")

        def create():
            self.X = dpg.get_value("X")
            self.Y = dpg.get_value("Y")
            dpg.show_item("plot_node")

            if self.X == "Humidité %":
                dpg.set_item_label("node_first_serie", "Humidité %")
                dpg.set_item_label("y_axis2", "Humidité %")
            elif self.X == "Température °C": 
                dpg.set_item_label("node_first_serie", "Température °C")
                dpg.set_item_label("y_axis2", "Température °C")
            elif self.X == "eCO2 (ppm)": 
                dpg.set_item_label("node_first_serie", "eCO2 (ppm)")
                dpg.set_item_label("y_axis2", "eCO2 (ppm)")
            elif self.X == "TVOC (ppb)": 
                dpg.set_item_label("node_first_serie", "TVOC (ppb)")
                dpg.set_item_label("y_axis2", "eCO2 (ppm)")

            if self.Y == "Humidité %":
                dpg.set_item_label("second", "Humidité %")
                dpg.set_item_label("y_axis3", "Humidité %")
            elif self.Y == "Température °C": 
                dpg.set_item_label("second", "Température °C")
                dpg.set_item_label("y_axis3", "Température °C")
            elif self.Y == "eCO2 (ppm)": 
                dpg.set_item_label("second", "eCO2 (ppm)")
                dpg.set_item_label("y_axis3", "eCO2 (ppm)")
            elif self.Y == "TVOC (ppb)": 
                dpg.set_item_label("second", "TVOC (ppb)")
                dpg.set_item_label("y_axis3", "TVOC (ppb)")

        # callback runs when user attempts to disconnect attributes
        def delink_callback(sender, app_data):
            # app_data -> link_id
            dpg.delete_item(app_data)

        with dpg.window(label="Tutorial", width=400, height=400):

            with dpg.node_editor(callback=link_callback, delink_callback=delink_callback):
                with dpg.node(label="Node 1"):
                    with dpg.node_attribute(label="Node A1", attribute_type=dpg.mvNode_Attr_Output):
                        dpg.add_combo(label="X", items=["Humidité %", "Température °C", "eCO2 (ppm)", "TVOC (ppb)"], tag="X")
                        dpg.add_combo(label="Y", items=["Humidité %", "Température °C", "eCO2 (ppm)", "TVOC (ppb)"], tag="Y")

                with dpg.node(label="Node 2"):
                    with dpg.node_attribute(label="Node A3"):
                        dpg.add_text("y=f(x)", tag="fonction_output")
                        dpg.add_button(label="create the plot", width=200, callback=create)

    def clb_selectable(self, sender, app_data, user_data):
        if dpg.does_item_exist(user_data):
            dpg.delete_item(user_data)

    def getData(self):
        # cette fonction permet 
        xi = float(dpg.get_value("dline2"))
        xf = float(dpg.get_value("dline3"))
        num = []
        data1 = []
        data2 = []
        finalData1 = [0] * 8  # [début, fin, max, min, moyenne, deltaS, deltaM]
        finalData2 = [0] * 8

        # Récupération des indices dans l'intervalle [xi, xf]
        for i, t in enumerate(time_dataPlot):
            if xi < t < xf:
                num.append(i)

        # Récupération des données
        if self.X == "Humidité %":
            for i in num:
                data1.append(humidity_data[i])

        elif self.X == "Température °C": 
            for i in num:
                data1.append(temperature_data[i])

        elif self.X == "eCO2 (ppm)": 
            for i in num:
                data1.append(co2_data[i])

        elif self.X == "TVOC (ppb)": 
            for i in num:
                data1.append(tvoc_data[i])

        if self.Y == "Humidité %":
            for i in num:
                data2.append(humidity_data[i])

        elif self.Y == "Température °C":
            for i in num:
                data2.append(temperature_data[i]) 

        elif self.Y == "eCO2 (ppm)": 
            for i in num:
                data2.append(co2_data[i])

        elif self.Y == "TVOC (ppb)":
            for i in num:
                data2.append(temperature_data[i]) 


        if data1:  # vérifier que data1 n'est pas vide
            finalData1[0] = self.X
            finalData1[1] = data1[1]
            finalData1[2] = data1[-1]
            finalData1[3] = max(data1)
            finalData1[4] = min(data1)
            total = sum(data1)
            finalData1[5] = total / len(data1)
            finalData1[6] = finalData1[2] - finalData1[1]
            finalData1[7] = finalData1[3] - finalData1[4]

        if data2:  # vérifier que data1 n'est pas vide
            finalData2[0] = self.Y
            finalData2[1] = data2[1]
            finalData2[2] = data2[-1]
            finalData2[3] = max(data2)
            finalData2[4] = min(data2)
            total = sum(data2)
            finalData2[5] = total / len(data2)
            finalData2[6] = finalData2[2] - finalData2[1]
            finalData2[7] = finalData2[3] - finalData2[4]

        row_tag = f"data_row_{self.next_row_id}"
        self.next_row_id += 1

        with dpg.table_row(parent="dataChoosen", tag=row_tag):
            dpg.add_text(finalData1[0])
            dpg.add_text(f"{finalData1[1]:.2f}")
            dpg.add_text(f"{finalData1[2]:.3f}")
            dpg.add_text(f"{finalData1[3]:.4f}")  
            dpg.add_text(f"{finalData1[4]:.5f}")
            dpg.add_text(f"{finalData1[5]:.6f}")  
            dpg.add_text(f"{finalData1[6]:.7f}")
            dpg.add_text(f"{finalData1[7]:.7f}")   
            dpg.add_selectable(label="del", span_columns=False, callback=self.clb_selectable, user_data=row_tag)

        row_tag = f"data_row_{self.next_row_id}"
        self.next_row_id += 1

        with dpg.table_row(parent="dataChoosen", tag=row_tag):
            dpg.add_text(finalData2[0])
            dpg.add_text(f"{finalData2[1]:.2f}")
            dpg.add_text(f"{finalData2[2]:.3f}")
            dpg.add_text(f"{finalData2[3]:.4f}")  
            dpg.add_text(f"{finalData2[4]:.5f}")
            dpg.add_text(f"{finalData2[5]:.6f}")  
            dpg.add_text(f"{finalData2[6]:.7f}")
            dpg.add_text(f"{finalData2[7]:.7f}")   
            dpg.add_selectable(label="del", span_columns=False, callback=self.clb_selectable, user_data=row_tag)


    def uptdate(self, timedata, whatdata, a):
        if whatdata == plot_win.X:
            dpg.set_value("node_first_serie", [timedata, a])
        if whatdata == plot_win.Y:
            dpg.set_value("second", [timedata, a])

    def add_row(self, table_tag, t, val):
        with dpg.table_row(parent=table_tag):
            dpg.add_text(f"{t:.1f}")
            dpg.add_text(f"{val:.2f}")

    def plot_canvas(self):
        """Exporter le graphique via Matplotlib"""
        fig = plt.figure(figsize=(11.69, 8.26), dpi=100)
        canvas = FigureCanvasAgg(fig)

        with lock:
            sns.lineplot(x=time_dataPlot, y=tvoc_dataPlot, label="TVOC (ppb)", linewidth=3)
            sns.lineplot(x=time_dataPlot, y=co2_dataPlot, label="eCO2 (ppm)", linewidth=3)

        plt.title("Mesures TVOC et CO2")
        plt.xlabel("Temps (s)")
        plt.ylabel("Valeur")
        canvas.draw()
        buf = canvas.buffer_rgba()
        image = np.asarray(buf).astype(np.float32) / 255

        if dpg.does_item_exist("matplotlib_win"):
            dpg.delete_item("matplotlib_win")
            dpg.delete_item("plot_texture")

        with dpg.texture_registry():
            dpg.add_raw_texture(1169, 826, image, format=dpg.mvFormat_Float_rgba, tag="plot_texture")

        with dpg.window(label="Matplotlib", pos=(325, 50), width=1200, height=900, tag="matplotlib_win"):
            dpg.add_image("plot_texture")
    
    def resetValue(self, table_tag):
        with lock:
            # Supprimer les lignes uniquement de la table concernée
            children = dpg.get_item_children(table_tag, 1)
            if children:
                for row in children:
                    dpg.delete_item(row)

            # Et vider les données correspondantes
            if table_tag == self.table_tvoc:
                tvoc_data.clear()
                time_data.clear()
            elif table_tag == self.table_co2:
                co2_data.clear()
                time_data.clear()



        
plot_win = PlotWin()

# ===========================================
# THREAD DE LECTURE SÉRIE
# ===========================================


def read_serial():
    serial_buffer = []
    try:
        # definis le port série
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        while dpg.is_dearpygui_running():
            line = ser.readline().decode("utf-8", errors="ignore").strip()     

            if line != "":
                with lock:
                        serial_buffer.append(line)  # ajouter la ligne au buffer
                        # Si on a reçu 4 lignes, les afficher toutes ensemble
                        if len(serial_buffer) >= 4:
                            # Concaténer les 4 lignes avec un retour à la ligne
                            message_block = "\n".join(serial_buffer[:4])
                            dpg.set_value(plot_win.serialPort, message_block)
                            # Supprimer les lignes affichées
                            serial_buffer = serial_buffer[4:]
                # selectionne les données si les valeurs de Co2 et de TVOC sont présent
                if "TVOC" in line and "CO2" in line:
                    # ajoute une composante temporelle 
                    t = time.time() - start_time
                    time_data.append(t)
                    time_dataPlot.append(t)
                    # recupère le texte transmis
                    parts = line.split(" ")
                    number = []
                    for i in parts:
                        x = i.split(":")
                        number.append(x[1])
                    
                    try:
                        # assigne a chaque indice de qualité de l'air la valeur transmise 
                        tvoc = float(number[2])
                        co2 = float(number[3])
                        temp = float(number[0])
                        hum = float(number[1])

                        with lock:

                            if temp == 0 or hum == 0:
                                return
                            # ajoute les valeurs aux listes
                            tvoc_data.append(tvoc)
                            tvoc_dataPlot.append(tvoc)
                            co2_data.append(co2)
                            co2_dataPlot.append(co2)
                            temperature_data.append(temp)
                            temperature_dataPlot.append(temp)
                            humidity_data.append(hum)
                            humidity_dataPlot.append(hum)
                            
                            # ajoute les valeurs aux tableaux
                            plot_win.add_row(plot_win.table_tvoc, t, tvoc)
                            plot_win.add_row(plot_win.table_co2, t, co2)
                            plot_win.add_row(plot_win.table_hum, t, hum)
                            plot_win.add_row(plot_win.table_temp, t, temp)

                            # ajoute les valeurs aux séries pour les graphiques
                            dpg.set_value('tvoc_series', [time_dataPlot, tvoc_dataPlot])
                            dpg.set_value('co2_series', [time_dataPlot, co2_dataPlot])
                            dpg.set_value('Température_series', [time_dataPlot, temperature_dataPlot])
                            dpg.set_value('Humidité_series', [time_dataPlot, humidity_dataPlot])

                            # uptdate les graphiques
                            plot_win.uptdate(time_dataPlot, "eCO2 (ppm)", co2_dataPlot)
                            plot_win.uptdate(time_dataPlot, "TVOC (ppb)", tvoc_dataPlot)
                            plot_win.uptdate(time_dataPlot, "Température °C", temperature_dataPlot)
                            plot_win.uptdate(time_dataPlot, "Humidité %", humidity_dataPlot)

                    except ValueError:
                        continue

                    
    except Exception as e:
        print(f"Erreur port série : {e}")


# ===========================================
# THREAD DE MISE À JOUR GRAPHIQUE
# ===========================================
def update_plot_thread():
    while dpg.is_dearpygui_running():
        time.sleep(0.2)


# ===========================================
# LANCEMENT
# ===========================================
threading.Thread(target=read_serial, daemon=True).start()
threading.Thread(target=update_plot_thread, daemon=True).start()

dpg.setup_dearpygui()
dpg.set_primary_window("main_win", True)
dpg.show_viewport()

while dpg.is_dearpygui_running():
    dpg.render_dearpygui_frame()

dpg.destroy_context()
