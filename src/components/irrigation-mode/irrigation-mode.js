import React from 'react';
import ToggleButton from 'react-toggle-button';
import './irrigation-mode.css';

const IrrigationMode = ({ value, onToggle, disabled }) => {
  return (
    <div className="irrigation-mode">
      <ToggleButton
        value={value}
        onToggle={onToggle}
        disabled={disabled}
      />
    </div>
  );
};

export default IrrigationMode;
