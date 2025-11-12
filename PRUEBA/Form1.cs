using System;
using System.Linq;
using System.Windows.Forms;
using System.IO.Ports;
using MySql.Data.MySqlClient;

namespace PRUEBA
{
    public partial class Form1 : Form
    {
        string conexionSql = "Server=localhost;Database=contador;port=3306;Uid=root;Pwd=*";
        private SerialPort ArduinoPort;

        public Form1()
        {
            InitializeComponent();

            ArduinoPort = new SerialPort();
            ArduinoPort.PortName = "COM5";
            ArduinoPort.BaudRate = 9600;
            ArduinoPort.DataBits = 8;
            ArduinoPort.ReadTimeout = 500;
            ArduinoPort.WriteTimeout = 500;
            ArduinoPort.DataReceived += new SerialDataReceivedEventHandler(DataReceivedHandler);
        }

        private void InsertarFrutaAcep(string cantidad, DateTime act)
        {
            using (MySqlConnection conect = new MySqlConnection(conexionSql))
            {
                conect.Open();

                string insertQ = "INSERT INTO frutaaceptada (numero, fecha_subida) VALUES (@Numero, @Fecha_subida)";

                using (MySqlCommand cmd = new MySqlCommand(insertQ, conect))
                {
                    cmd.Parameters.AddWithValue("@Numero", cantidad);
                    cmd.Parameters.AddWithValue("@Fecha_subida", act);
                    cmd.ExecuteNonQuery();
                }
                conect.Close();
            }
        }

        private void InsertarFrutaDen(string cantidad, DateTime act)
        {
            using (MySqlConnection conn = new MySqlConnection(conexionSql))
            {
                conn.Open();
                string insertQ = "INSERT INTO frutarechazada(numero, fecha_subida) VALUES(@Numero, @Fecha_subida)";

                using (MySqlCommand cmd = new MySqlCommand(insertQ, conn))
                {
                    cmd.Parameters.AddWithValue("@Numero", cantidad) ;
                    cmd.Parameters.AddWithValue("@Fecha_subida", act) ;
                    cmd.ExecuteNonQuery();
                }
                conn.Close();
            }
        }

        private void DataReceivedHandler(object sender, SerialDataReceivedEventArgs e)
        {
            try
            {
                string dato = ArduinoPort.ReadLine();

                // Actualizar de forma segura desde otro hilo
                this.Invoke(new Action(() => {
                    Txt(dato);
                }));
            }
            catch (Exception ex)
            {
                // Manejar errores de lectura
                System.Diagnostics.Debug.WriteLine("Error: " + ex.Message);
            }
        }

        private void Txt(string dato)
        {
            // Detectar mensaje de área mayor a 2cm² y actualizar lblNum
            if (dato.Contains("AREA_MAYOR:1"))
            {
                try
                {
                    int num = int.Parse(lblNum.Text);
                    num++;
                    lblNum.Text = num.ToString();

                    // Opcional: Log para debug
                    System.Diagnostics.Debug.WriteLine($"Fruta > 2cm² detectada. Total: {num}");
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine("Error al actualizar contador: " + ex.Message);
                }
            }

            // NUEVO: Detectar mensaje de área menor o igual a 2cm² y actualizar lblNumNoAcep
            if (dato.Contains("AREA_MENOR:1"))
            {
                try
                {
                    int numNoAcep = int.Parse(lblNumNoAcep.Text);
                    numNoAcep++;
                    lblNumNoAcep.Text = numNoAcep.ToString();

                    // Opcional: Log para debug
                    System.Diagnostics.Debug.WriteLine($"Fruta <= 2cm² detectada. Total: {numNoAcep}");
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine("Error al actualizar contador no aceptadas: " + ex.Message);
                }
            }
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            // Intentar encontrar el puerto del Arduino automáticamente
            string[] puertos = SerialPort.GetPortNames();

            if (puertos.Length == 0)
            {
                MessageBox.Show("No se detectaron puertos COM disponibles.\nAsegúrate de que el Arduino está conectado.",
                    "Error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            // Mostrar los puertos disponibles y seleccionar
            string mensaje = "Puertos disponibles:\n" + string.Join("\n", puertos);
            MessageBox.Show(mensaje, "Puertos COM detectados", MessageBoxButtons.OK, MessageBoxIcon.Information);

            // Intentar con el primer puerto disponible o COM5
            string puertoSeleccionado = puertos.Contains("COM5") ? "COM5" : puertos[0];

            try
            {
                ArduinoPort.PortName = puertoSeleccionado;

                if (!ArduinoPort.IsOpen)
                {
                    ArduinoPort.Open();
                    lblNum.Text = "0";
                    MessageBox.Show($"Conectado exitosamente a {puertoSeleccionado}",
                        "Conexión exitosa", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error al abrir {puertoSeleccionado}:\n{ex.Message}\n\n" +
                    "Posibles soluciones:\n" +
                    "1. Cierra el Monitor Serial del Arduino IDE\n" +
                    "2. Verifica que el Arduino esté conectado\n" +
                    "3. Intenta desconectar y reconectar el Arduino",
                    "Error de conexión", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }


        private void btnConectar_Click(object sender, EventArgs e)
        {
            try
            {
                if (!ArduinoPort.IsOpen)
                {
                    ArduinoPort.Open();
                    btnConectar.Text = "Desconectar";
                }
                else
                {
                    ArduinoPort.Close();
                    btnConectar.Text = "Conectar";
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error: " + ex.Message, "Error de conexión",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void btnReset_Click_1(object sender, EventArgs e)
        {
            ArduinoPort.Write("E");
            lblNum.Text = "0";
        }

        private void btnSubir_Click(object sender, EventArgs e)
        {
            string datos = $"Cantidad de frutas aceptadas: {lblNum.Text}\n" +
                $"Cantidad de frutas rechazadas: {lblNumNoAcep.Text}";

            try
            {
                InsertarFrutaAcep(lblNum.Text, DateTime.Now);
                InsertarFrutaDen(lblNumNoAcep.Text, DateTime.Now);

                MessageBox.Show("Datos guardados exitosamente:\n\n" + datos,
                    "Información de registro", MessageBoxButtons.OK, MessageBoxIcon.Information);

                ArduinoPort.Write("E");
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error al guardar: " + ex.Message,
                    "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
    }
}