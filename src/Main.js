import React, { useState, useEffect } from 'react';
import MoistureSensor from './components/moisture-sensor/MoistureSensor';
import "./style.css"
import Card from './shared/Card/Card';
import IrrigationMode from './components/irrigation-mode/irrigation-mode';
import TimePickerComponent from './components/time-picker/time-picker';
import HumidityChart from './components/chart/irrigation-chart';

const App = () => {
  const [modeOn, setModeOn] = useState(false);
  const [updatingMode, setUpdatingMode] = useState(false);
  const [sensorData, setSensorData] = useState(null);
  const [sensorMode, setSensorMode] = useState(false); 
  const [updatingSensorMode, setUpdatingSensorMode] = useState(false);

  const setModeState = (state) => {
    setModeOn(state !== '0');
    setUpdatingMode(false);
  };

  const setSensorModeState = (state) => {
    setSensorMode(state !== '0');
    setUpdatingSensorMode(false);
  };

  const handleStateChange = (modeOn) => {
    setUpdatingMode(true);

    fetch('/mode', { method: 'PUT', body: modeOn ? '0' : '1', timeout: 1000 })
      .then(response => response.text())
      .then(state => setModeState(state))
      .catch(error => {
        console.error('Erro no request do mode:', error);
        setUpdatingMode(false);
      });
  };

  const handleSensorModeChange = (sensorMode) => {
    setUpdatingSensorMode(true);

    fetch('/sensorMode', { method: 'PUT', body: sensorMode ? '0' : '1', timeout: 1000 })
      .then(response => response.text())
      .then(sensorMode => {
        setSensorModeState(sensorMode);
      })
      .catch(error => {
        console.error('Erro no request do sensorMode:', error);
        setSensorModeState(false);
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
      .then(state => setModeState(state))
      .catch(error => {
        console.error('Erro ao obter dados do rele:', error);
      });
  };

  const checkSensorModeStatus = () => {
    fetch('/sensorMode')
      .then(response => response.text())
      .then(state => setSensorModeState(state))
      .catch(error => {
        console.error('Erro ao obter dados do modo do sensor:', error);
      });
  };

  useEffect(() => {
    checkModeStatus();
    fetchMoistureData();
    const intervalID = setInterval(() => {
      checkModeStatus();
      fetchMoistureData();
      checkSensorModeStatus();
    }, 1000);

    // Limpeza quando o componente é desmontado
    return () => clearInterval(intervalID);
  }, []); // O array vazio assegura que o efeito só é executado uma vez após a montagem do componente

  return (
    <div>
      <div className="main">
        <div className="container row card-container">
          <Card title="Modo da Irrigação" body={<IrrigationMode modeValue={modeOn} sensorModeValue={sensorMode} onModeToggle={handleStateChange} onSensorModeToggle={handleSensorModeChange} disabled={updatingMode || updatingSensorMode} />} />
          <Card title="Agendamento" body={<TimePickerComponent />} />
          <Card title="Umidade do Solo" body={<MoistureSensor sensorData={sensorData} />} />
          <Card title="Monitoramento" body={<HumidityChart />} />
        </div>
      </div>
    </div>
  );
}

export default App;
