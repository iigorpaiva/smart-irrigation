import React, { useState, useEffect } from 'react';
import MoistureSensor from './components/moisture-sensor/MoistureSensor';
import "./style.css"
import Card from './shared/Card/Card';
import IrrigationMode from './components/irrigation-mode/irrigation-mode';
import TimePickerComponent from './components/time-picker/time-picker';
import HumidityChart from './components/chart/irrigation-chart';

const App = () => {
  const [ledOn, setLedOn] = useState(false);
  const [updating, setUpdating] = useState(false);
  const [sensorData, setSensorData] = useState(null);

  const setLedState = (state) => {
    setLedOn(state !== '0');
    setUpdating(false);
  };

  const handleStateChange = (ledOn) => {
    setUpdating(true);

    fetch('/mode', { method: 'PUT', body: ledOn ? '0' : '1', timeout: 1000 })
      .then(response => response.text())
      .then(state => setLedState(state))
      .catch(error => {
        console.error('Erro no request do mode:', error);
        setUpdating(false);
      });
  };

  const fetchMoistureData = async () => {
    try {
      const response = await fetch('/moisture'); 
      const sensorData = await response.json();
      setSensorData(sensorData);
    } catch (error) {
      console.error('Erro ao obter dados do sensor:', error);
    }
  };

  const checkModeStatus = () => {
    fetch('/mode')
      .then(response => response.text())
      .then(state => setLedState(state))
      .catch(error => {
        console.error('Erro ao obter dados do rele:', error);
      });
  };

  useEffect(() => {
    checkModeStatus();
    fetchMoistureData();
    const intervalID = setInterval(() => {
      checkModeStatus();
      fetchMoistureData();
    }, 1000);

    // Limpeza quando o componente é desmontado
    return () => clearInterval(intervalID);
  }, []); // O array vazio assegura que o efeito só é executado uma vez após a montagem do componente

  return (
    <div>
      <div className="main">
        <div className="container row card-container">
          <Card title="Modo da Irrigação" body={<IrrigationMode value={ledOn} onToggle={value => handleStateChange(value)} disabled={updating} />} />
          <Card title="Agendamento" body={<TimePickerComponent />} />
          <Card title="Umidade do Solo" body={<MoistureSensor sensorData={sensorData} />} />
          <Card title="Monitoramento" body={<HumidityChart />} />
        </div>
      </div>
    </div>
  );
}

export default App;
