// MoistureSensor.js
import React, { useEffect } from 'react';
import { LinearGauge, Title, Font, Geometry, Scale, RangeContainer, ValueIndicator } from 'devextreme-react/linear-gauge';
import "./MoistureSensor.css";

const MoistureSensor = ({ sensorData }) => {
  useEffect(() => {
    // console.log("SENSOR: ", sensorData);
  }, [sensorData]);

  return (
    <div className='moisture-sensor'>
      <LinearGauge
        className="gauge-element"
        value={sensorData} // Utilizando sensorData diretamente como valor da umidade
      >
        <Title text="Umidade (%)" style={{ fontSize: '16px', color: '#ADD8E6' }} >
          <Font size={12} />
        </Title>
        <Geometry orientation="vertical" />
        <Scale startValue={0} endValue={100} tickInterval={10} />
        <RangeContainer backgroundColor="#CACACA" />
        <ValueIndicator type="rhombus" color="#A4DDED" />
      </LinearGauge>
    </div>
  );
};

export default MoistureSensor;
