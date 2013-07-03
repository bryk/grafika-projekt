#version 330

in vec3 modelSpacePosition;
in vec4 diffuseColor;
in vec3 vertexNormal;

uniform	vec3 position; // Position in eye coords
uniform	vec3 position2; // Position in eye coords
uniform	vec3 intensity;
uniform	vec3 direction; // Direction of the spotlight in eye coords.
uniform	float exponent; // Angular attenuation exponent
uniform	float cutoff; // Cutoff angle (between 0 and 90)
uniform vec3 Kd; // Diffuse reflectivity
uniform vec3 Ka; // Ambient reflectivity
uniform vec3 Ks; // Specular reflectivity
uniform float Shininess; // Specular shininess factor

out vec4 outputColor;

vec4 adsWithSpotlight(vec3 position) {
	vec3 s = normalize(vec3( position) - modelSpacePosition );
	vec3 spotDir = normalize(direction);
	float angle = acos(dot(-s, spotDir));
	float cutoff = radians(clamp(cutoff, 0.0, 90.0));
	if (angle < cutoff) {
		float spotFactor = pow( dot(-s, spotDir), exponent );
		vec3 v = normalize(vec3(-modelSpacePosition));
		vec3 h = normalize( v + s );
		return vec4(spotFactor * intensity * (Kd * max( dot(s, vertexNormal), 0.0 ) +
			Ks * pow(  max(dot(h, vertexNormal), 0.0)   , Shininess) ), 1.0);
	} else { 
		return vec4(0.0, 0.0, 0.0, 0.0);
	}
}

void main() {
	vec4 amb = vec4(intensity * Ka, 1.0);
	outputColor = (amb + adsWithSpotlight(position) + adsWithSpotlight(position2))* diffuseColor;
}
