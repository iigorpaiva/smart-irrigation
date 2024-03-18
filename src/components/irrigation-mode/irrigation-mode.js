import React from 'react';
import ToggleButton from 'react-toggle-button';
import './irrigation-mode.css';

const IrrigationMode = ({ modeValue, sensorModeValue, onModeToggle, onSensorModeToggle, disabled }) => {
  return (
    <div className="irrigation-mode">
      <div style={{ display: 'flex', alignItems: 'center' }}>
        <h3 style={{ marginRight: '10px' }}>ligar/desligar: </h3>
        <ToggleButton
          value={modeValue}
          onToggle={onModeToggle}
          disabled={disabled}
        />
      </div>
      <div style={{ display: 'flex', alignItems: 'center' }}>
      <h3 style={{ marginRight: '10px' }}>considerar sensor: </h3>
      <ToggleButton
        value={sensorModeValue}
        onToggle={onSensorModeToggle}
        disabled={disabled}
      />
      </div>
    </div>
  );
};

export default IrrigationMode;
