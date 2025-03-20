// App.js
import React from 'react';
import { BrowserRouter as Router, Routes, Route, Navigate } from 'react-router-dom';
import { AuthProvider, useAuth } from './context/AuthContext';
import Sidebar from './Sidebar';
import DevicesPage from './pages/DevicesPage';
import ScenesPage from './pages/ScenesPage';
import SettingsPage from './pages/SettingsPage';
import LogsPage from './pages/LogsPage';
import MapEditorPage from './pages/MapEditorPage';
import LoginPage from './pages/LoginPage';
import ProtectedRoute from './ProtectedRoute';
import './App.css';
import { DevicesProvider } from './context/DevicesContext';

const AppRoutes = () => {
    const { authToken } = useAuth();
    return (
        <Routes>
            <Route path="/login" element={<LoginPage />} />
            <Route
                path="/*"
                element={
                    <ProtectedRoute isAuthenticated={Boolean(authToken)}>
                        <DevicesProvider>
                            <div className="dashboard">
                                <Sidebar />
                                <div className="main-content">
                                    <Routes>
                                        <Route path="/" element={<Navigate to="/devices" replace />} />
                                        <Route path="/devices" element={<DevicesPage />} />
                                        <Route path="/scenes" element={<ScenesPage />} />
                                        <Route path="/settings" element={<SettingsPage />} />
                                        <Route path="/logs" element={<LogsPage />} />
                                        <Route path="/map-editor" element={<MapEditorPage />} />
                                    </Routes>
                                </div>
                            </div>
                        </DevicesProvider>
                    </ProtectedRoute>
                }
            />
        </Routes>
    );
};

const App = () => {
    return (
        <AuthProvider>
            <Router>
                <AppRoutes />
            </Router>
        </AuthProvider>
    );
};

export default App;
