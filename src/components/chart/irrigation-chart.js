import React, { useEffect, useState } from 'react';
import { Chart, registerables } from 'chart.js';
import 'chartjs-adapter-moment';

Chart.register(...registerables);

// const generateMockData = () => {
//   const labels = [];
//   const values = [];

//   // Gera dados para as últimas 12 horas
//   for (let i = 12; i > 0; i--) {
//     const currentTime = new Date();
//     currentTime.setHours(currentTime.getHours() - i);

//     labels.push(currentTime.toISOString());
//     values.push(Math.floor(Math.random() * 100)); // Valor de umidade aleatório entre 0 e 100
//   }

//   return { labels, values };
// };

const HumidityChart = () => {
  const [chart, setChart] = useState(null);

  useEffect(() => {
    const fetchData = async () => {
      try {

        fetch('/chartData', { method: 'GET' })
          .then(response => response.text())
          .then(charData => {
            console.log("o chartdata resposta: ", charData.toString());
            // console.log("generated: ", generateMockData());
            // Parse a resposta como JSON
            const parsedData = JSON.parse(charData);

            // Criar arrays labels e values
            const labels = [];
            const values = [];

            // Itera sobre os pares chave-valor e adiciona aos arrays
            Object.entries(parsedData).forEach(([hour, value]) => {
              // Converte a hora para o formato desejado
              const currentTime = new Date();
              currentTime.setHours(parseInt(hour));
              const formattedTime = currentTime.toISOString();

              // Adiciona aos arrays
              labels.push(formattedTime);
              values.push(value);

              
            });

            // Inverte a ordem dos arrays para que as horas antigas apareçam à esquerda
            labels.reverse();
            values.reverse();

            // Crie o gráfico usando os dados obtidos do servidor
            const ctx = document.getElementById('humidity-chart').getContext('2d');
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
          })
          .catch(error => {
            console.error('Erro ao buscar o chartData', error);
          });
      } catch (error) {
        console.error('Erro ao buscar dados do servidor:', error);
      }
    };

    // Chama a função para buscar os dados
    fetchData();
  }, []); // O array vazio assegura que o efeito só é executado uma vez após a montagem do componente

  return (
    <div className="irrigation-chart-container">
      <canvas id="humidity-chart"></canvas>
    </div>
  );
};

export default HumidityChart;
