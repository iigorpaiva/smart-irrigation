import React, { useEffect, useState } from 'react';
import TimePicker from 'rc-time-picker';
import 'rc-time-picker/assets/index.css';
import "./time-picker.css";

const TimePickerComponent = () => {
  const [numTimePickers, setNumTimePickers] = useState(1);
  const [irrigationDuration, setIrrigationDuration] = useState(15);
  const [scheduleList, setScheduleList] = useState(Array.from({ length: 4 }, () => ({ time: null, duration: irrigationDuration })));

  const handleNumTimePickersChange = (e) => {
    const selectedNum = parseInt(e.target.value, 10);
    setNumTimePickers(Math.min(selectedNum, 4));
  };

  useEffect(() => {
    try {
      fetch('/duration', { method: 'GET' })
        .then(response => response.text())
        .then(duration => {
          console.log('Foi salvo no servidor a duração: ', duration);
          
          // Atualizar o estado de irrigationDuration no componente
          setIrrigationDuration(parseInt(duration, 10));
        })
        .catch(error => {
          console.error('Erro ao buscar o duration:', error);
        });
    } catch (error) {
      console.error('Erro ao buscar o duration:', error);
    }
  }, []);

  const handleIrrigationDurationChange = (e) => {
    const selectedDuration = parseInt(e.target.value, 10);

    try {
      fetch('/duration', { method: 'POST', body: JSON.stringify(selectedDuration) })
        .then(response => response.text())
        .then(duration => {
          
          // Atualizar o estado de irrigationDuration no componente
          setIrrigationDuration(parseInt(duration, 10));
        })
        .catch(error => {
          console.error('Erro no POST da duração:', error);
        });
    } catch (error) {
      console.error('Erro ao realizar o cadastro da duração:', error);
    }
  };

  const handleTimeChange = (time, index) => {
    // Converte o horário local para UTC antes de enviar
    const adjustedTime = time && time.utcOffset(0, true);
  
    const newScheduleList = [...scheduleList];
    newScheduleList[index] = { time: adjustedTime };
    setScheduleList(newScheduleList);
  
    // Envie a lista atualizada para o ESP32
    sendTimesToESP32(newScheduleList);
  };  

  const sendTimesToESP32 = async (timesList) => {
    // Filtrar apenas os itens com 'time' não nulo
    const validTimesList = timesList.filter(item => item.time !== null);

    try {

      fetch('/schedule', { method: 'POST', body: JSON.stringify(validTimesList) })
        .then(response => response.text())
        .then(data => {
          console.log('Resposta do servidor:', data);
          // Aqui você pode realizar qualquer outra lógica com os dados retornados
        })
        .catch(error => {
          console.error('Erro no POST do schedule:', error);
      });

    } catch (error) {
      console.error('Erro ao realizar a requisição:', error);
    }
  };

  const renderTimePickers = () => {
    const timePickers = [];
    for (let i = 0; i < numTimePickers; i++) {
      const selectedTime = scheduleList[i];
      timePickers.push(
        <div key={i} className="time-picker-item">
          <div className="time-picker-label">
            <h3>Horário {i + 1}:</h3>
          </div>
          <div className="time-picker-input">
            <TimePicker showSecond={false} value={selectedTime ? selectedTime.time : null} onChange={(time) => handleTimeChange(time, i)} />
          </div>
        </div>
      );
    }
    return timePickers;
  };

  return (
    <div className="time-picker-container">
      <div className="time-picker-container-in-line">
        <div className="dropdown-container">
          <h3>Número de Irrigações: </h3>
          <select id="numTimePickers" value={numTimePickers} onChange={handleNumTimePickersChange}>
            {[1, 2, 3, 4].map((value) => (
              <option key={value} value={value}>
                {value}
              </option>
            ))}
          </select>
        </div>
        <div className="dropdown-container">
          <h3>Duração das Irrigações:</h3>
          <select id="irrigationDuration" value={irrigationDuration} onChange={handleIrrigationDurationChange}>
            {[15, 30, 45, 60, 75, 90, 105, 120].map((value) => (
              <option key={value} value={value}>
                {value} minutos
              </option>
            ))}
          </select>
        </div>
        <div className="time-pickers">
          {renderTimePickers()}
        </div>
      </div>
    </div>
  );
};

export default TimePickerComponent;
