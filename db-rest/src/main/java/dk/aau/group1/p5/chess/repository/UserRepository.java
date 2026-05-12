package dk.aau.group1.p5.chess.repository;
import java.util.List;

import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;

import dk.aau.group1.p5.chess.model.User;

public interface UserRepository extends JpaRepository<User, Long> {
    
    @Query(
        value = "SELECT * FROM users u WHERE u.authentik_user_id = ?1",
        nativeQuery = true
    )
    public List<User> getByAuthentikUserId(String id);
}
