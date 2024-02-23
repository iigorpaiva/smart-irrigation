import React, { useEffect, useState } from 'react';
import TimePicker from 'rc-time-picker';
import 'rc-time-picker/assets/index.css';
import "./time-picker.css";

import moment from 'moment';
import 'moment/locale/pt-br'; // ou outra localização desejada

const TimePickerComponent = () => {
  const [numTimePickers, setNumTimePickers] = useState(1);
  const [irrigationDuration, setIrrigationDuration] = useState(15);
  const [scheduleList, setScheduleList] = useState(Array.from({ length: 4 }, () => ({ time: null, duration: irrigationDuration })));

  const handleNumTimePickersChange = (e) => {
    const selectedNum = parseInt(e.target.value, 10);
    const newScheduleList = scheduleList.slice(0, selectedNum);
  
    // Atualizar o estado de scheduleList no componente
    setScheduleList(newScheduleList);
  
    // Atualizar o estado de numTimePickers com base na quantidade de horários recebidos
    setNumTimePickers(selectedNum);
  
    // Envie a lista atualizada para o ESP32
    sendTimesToESP32(newScheduleList);
  };

  useEffect(() => {
    try {
      fetch('/duration', { method: 'GET' })
      .then(response => response.text())
      .then(duration => {
        console.log('Foi salvo no servidor a duração: ', duration);
    
        // Atualizar o estado de irrigationDuration no componente
        const parsedDuration = parseInt(duration, 10);
        setIrrigationDuration(parsedDuration || 15); // Se parsedDuration for falsy, define como 15
      })
      .catch(error => {
        console.error('Erro ao buscar o duration:', error);
      });

      fetch('/schedule', { method: 'GET' })
        .then(response => response.text())
        .then(schedule => {
          console.log("HORÁRIOS: ", schedule);
      
          // Verificar se o schedule está nulo ou indefinido
          if (schedule == null || schedule.trim().toLowerCase() === 'null' || schedule === "Not Found") {

            // Trate o caso especial aqui, como definir um valor padrão para receivedScheduleList
            const receivedScheduleList = []; // ou qualquer valor padrão desejado
            setNumTimePickers(1); // Pode ser 0 ou qualquer valor que faça sentido para o seu caso
          } else {
            // Caso normal: processar o schedule recebido
            const receivedScheduleList = schedule.split('\n').map(time => ({ time: time.trim() }));
            const filteredScheduleList = receivedScheduleList.filter(item => item.time !== '');
            console.log("FILTRADOS: ", filteredScheduleList);
      
            // Atualizar o estado de scheduleList no componente
            setScheduleList(receivedScheduleList);
      
            // Atualizar o estado de numTimePickers com base na quantidade de horários recebidos
            setNumTimePickers(filteredScheduleList.length);
          }
        })
        .catch(error => {
          console.error('Erro ao buscar os horários:', error);
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
    // Converta o horário local para UTC antes de enviar
    const adjustedTime = time && time.utcOffset(0, true).format('HH:mm');
    
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
      const formattedTime = selectedTime && selectedTime.time
        ? moment.utc(selectedTime.time, 'HH:mm')
        : null;
  
      timePickers.push(
        <div key={i} className="time-picker-item">
          <div className="time-picker-label">
            <h3>Horário {i + 1}:</h3>
          </div>
          <div className="time-picker-input">
            <TimePicker
              showSecond={false}
              value={formattedTime}
              onChange={(time) => handleTimeChange(time, i)}
            />
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
          <h3>Número de Irrigações diárias: </h3>
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
