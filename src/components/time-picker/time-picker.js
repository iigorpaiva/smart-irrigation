import React from 'react';
import TimePicker from 'rc-time-picker';
import 'rc-time-picker/assets/index.css';
import Logo from '../logo/logo';
import "./time-picker.css"

const TimePickerComponent = () => {
  return (
    <div className="time-picker-container">
      <div className="logo-container">
        <Logo />
      </div>
      <div className="time-picker">
        <TimePicker />
        <TimePicker />
      </div>
    </div>
  );
};

export default TimePickerComponent;
