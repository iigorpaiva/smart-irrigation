import React, { useEffect, useState } from 'react';
import { Chart, registerables } from 'chart.js';
import 'chartjs-adapter-moment';

Chart.register(...registerables);

const generateMockData = () => {
  const labels = [];
  const values = [];

  // Gera dados para as últimas 12 horas
  for (let i = 12; i > 0; i--) {
    const currentTime = new Date();
    currentTime.setHours(currentTime.getHours() - i);

    labels.push(currentTime.toISOString());
    values.push(Math.floor(Math.random() * 100)); // Valor de umidade aleatório entre 0 e 100
  }

  return { labels, values };
};

const HumidityChart = () => {
  const [chart, setChart] = useState(null);

  useEffect(() => {
    const mockData = generateMockData();

    const ctx = document.getElementById('humidity-chart').getContext('2d');

    const newChart = new Chart(ctx, {
      type: 'bar',
      data: {
        labels: mockData.labels,
        datasets: [
          {
            label: 'Umidade (%)',
            data: mockData.values,
            borderColor: '#A4DDED',
            backgroundColor: '#A4DDED',
            borderWidth: 1,
          },
        ],
      },
      options: {
        scales: {
          x: {
            type: 'time',
            time: {
              unit: 'hour',
              displayFormats: {
                hour: 'HH:mm',
              },
            },
            title: {
              display: true,
              text: 'Tempo',
            },
          },
          y: {
            title: {
              display: true,
              text: 'Umidade (%)',
            },
            ticks: {
              beginAtZero: true,
              max: 100,
              min: 0,
            },
          },
        },
      },
    });

    setChart(newChart);

    // Adiciona um event listener para redimensionamento
    const handleResize = () => {
      newChart.resize();
    };

    window.addEventListener('resize', handleResize);

    return () => {
      window.removeEventListener('resize', handleResize);
      newChart.destroy();
    };
  }, []); // O array vazio assegura que o efeito só é executado uma vez após a montagem do componente

  return (
    <div className="irrigation-chart-container">
      <canvas id="humidity-chart"></canvas>
    </div>
  );
};

export default HumidityChart;
