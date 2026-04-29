package dk.aau.group1.p5.chess.config;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.security.config.annotation.web.builders.HttpSecurity;
import org.springframework.security.web.SecurityFilterChain;
import org.springframework.security.web.util.matcher.RequestMatcher;

@Configuration
public class SecurityConfig {

    @Bean
    SecurityFilterChain securityFilterChain(HttpSecurity http) throws Exception {

        // 👇 PUT YOUR MATCHER HERE
        RequestMatcher localhostDirectOnly = request -> {
            String remoteAddr = request.getRemoteAddr();
            String forwardedFor = request.getHeader("X-Forwarded-For");

            boolean fromLocalSocket =
                "127.0.0.1".equals(remoteAddr)
                || "::1".equals(remoteAddr)
                || "0:0:0:0:0:0:0:1".equals(remoteAddr);

            boolean noProxyClient =
                forwardedFor == null || forwardedFor.isBlank();

            return fromLocalSocket && noProxyClient;
        };

        // 👇 USE IT HERE
        return http
            .csrf(csrf -> csrf.disable())
            .authorizeHttpRequests(auth -> auth
                .requestMatchers(localhostDirectOnly).permitAll()
                .anyRequest().authenticated()
            )
            .oauth2ResourceServer(oauth2 -> oauth2.jwt())
            .build();
    }
}