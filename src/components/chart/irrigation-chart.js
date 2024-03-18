import React, { useEffect, useState, useRef } from 'react';
import { Chart, registerables } from 'chart.js';
import 'chartjs-adapter-moment';

Chart.register(...registerables);

const HumidityChart = () => {
  const chartRef = useRef(null);

  const createChart = (labels, values) => {
    const ctx = document.getElementById('humidity-chart').getContext('2d');

    // Destroi o gráfico anterior se existir
    if (chartRef.current) {
      chartRef.current.destroy();
    }

    const newChart = new Chart(ctx, {
      type: 'bar',
      data: {
        labels: labels,
        datasets: [
          {
            label: 'Umidade (%)',
            data: values,
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
              max: 100,         // Definir o máximo explicitamente como 100
              min: 0,           // Definir o mínimo explicitamente como 0
              stepSize: 10,     // Opcional: definir um incremento de escala
            },
          },
        },
      },
    });

    chartRef.current = newChart;

    const handleResize = () => {
      newChart.resize();
    };

    window.addEventListener('resize', handleResize);

    return () => {
      window.removeEventListener('resize', handleResize);
      newChart.destroy();
    };
  };

  const fetchData = async () => {
    try {
      const response = await fetch('/chartData', { method: 'GET' });
      const charData = await response.text();
      const parsedData = JSON.parse(charData);
  
      if (parsedData && typeof parsedData === 'object') {
        const labels = [];
        const values = [];
  
        Object.entries(parsedData).forEach(([hour, value]) => {
          const currentTime = new Date();
          currentTime.setHours(parseInt(hour));
          const formattedTime = currentTime.toISOString();
  
          labels.push(formattedTime);
          values.push(value);
        });
  
        labels.reverse();
        values.reverse();
  
        // Verifica se os dados são válidos antes de criar o gráfico
        if (labels.length > 0 && values.length > 0) {
          // Cria ou atualiza o gráfico
          createChart(labels, values);
        } else {
          console.error('Dados recebidos do servidor são inválidos:', parsedData);
        }
      } else {
        console.error('Dados recebidos do servidor são inválidos:', parsedData);
      }
    } catch (error) {
      console.error('Erro ao buscar dados do servidor:', error);
    }
  };
  

  useEffect(() => {
    fetchData();

    const intervalID = setInterval(() => {
      fetchData();
    }, 60000);

    // Limpeza quando o componente é desmontado
    return () => clearInterval(intervalID);
  }, []); 

  return (
    <div className="irrigation-chart-container">
      <canvas id="humidity-chart"></canvas>
    </div>
  );
};

export default HumidityChart;
