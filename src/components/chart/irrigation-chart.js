import React from 'react';
import { Bar } from 'react-chartjs-2';
import { Chart, registerables } from 'chart.js';

Chart.register(...registerables);

const HumidityChart = () => {
  // Dados fixos para simulação
  const labels = ['00:00', '01:00', '02:00', '03:00', '04:00', '05:00', '06:00', '07:00', '08:00', '09:00', '10:00', '11:00', '12:00', '13:00', '14:00', '15:00', '16:00', '17:00', '18:00', '19:00', '20:00', '21:00', '22:00', '23:00'];
  const data = [60, 70, 80, 90, 95, 85, 75, 70, 65, 65, 65, 60, 55, 40, 40, 40, 30, 30, 27, 25, 20, 20, 15, 10];

  const chartData = {
    labels: labels,
    datasets: [
      {
        label: 'Umidade (%)',
        data: data,
        borderColor: '#A4DDED',
        backgroundColor: '#A4DDED',
        borderWidth: 1,
      },
    ],
  };

  const options = {
    scales: {
      x: [
        {
          type: "time",
          time: {
            unit: "hour",
            displayFormats: {
              hour: "HH:mm",
            },
          },
          scaleLabel: {
            display: true,
            labelString: "Tempo",
          },
        },
      ],
      y: [
        {
          scaleLabel: {
            display: true,
            labelString: "Umidade (%)",
          },
          ticks: {
            beginAtZero: true,
            max: 100,
            min: 0,
          },
        },
      ],
    },
  };

  return (
    <div className="irrigation-chart-container">
      <Bar data={chartData} options={options} />
    </div>
  );
};

export default HumidityChart;
