// TimePickerWithLogo.js
import React, { useState } from 'react';
import TimePicker from 'rc-time-picker';
import 'rc-time-picker/assets/index.css';
import Logo from '../logo/logo';
import "./time-picker.css";

const TimePickerComponent = () => {
  const [numTimePickers, setNumTimePickers] = useState(1);
  const [irrigationDuration, setIrrigationDuration] = useState(15);

  const handleNumTimePickersChange = (e) => {
    const selectedNum = parseInt(e.target.value, 10);
    setNumTimePickers(Math.min(selectedNum, 5));
  };

  const handleIrrigationDurationChange = (e) => {
    const selectedDuration = parseInt(e.target.value, 10);
    setIrrigationDuration(selectedDuration);
  };

  const renderTimePickers = () => {
    const timePickers = [];
    for (let i = 0; i < numTimePickers; i++) {
      timePickers.push(
        <div key={i} className="time-picker-item">
          <div className="time-picker-label">
            <h3>Horário {i + 1}:</h3>
          </div>
          <div className="time-picker-input">
            <TimePicker showSecond={false} />
          </div>
        </div>
      );
    }
    return timePickers;
  };

  return (
    <div className="time-picker-container">
      <div className="logo-container">
        <Logo />
      </div>
      <div className="time-picker-container-in-line">
        <div className="dropdown-container">
          <h3>Número de Irrigações: </h3>
          <select id="numTimePickers" value={numTimePickers} onChange={handleNumTimePickersChange}>
            {[1, 2, 3, 4, 5].map((value) => (
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
