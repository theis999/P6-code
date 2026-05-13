package dk.aau.group1.p5.chess.web;

import java.util.ArrayList;
import java.util.List;

import org.springframework.web.bind.annotation.CrossOrigin;
import org.springframework.web.bind.annotation.RestController;

import dk.aau.group1.p5.chess.model.Board;
import dk.aau.group1.p5.chess.model.Game;
import dk.aau.group1.p5.chess.model.User;
import dk.aau.group1.p5.chess.service.UserService;

import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PatchMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;

@CrossOrigin(origins = "*")
@RestController
public class UserController {
    private final UserService service;

    public UserController(UserService userService) {
        service = userService;
    }

    @GetMapping("/users")
    public List<User> getAllUsers() {
        return service.getAll();
    }

    @PostMapping("/users")
    public User addUser(@RequestBody User new_user) {
        new_user.setId(null);

        return service.save(new_user);
    }

    @PatchMapping("/users/{id}")
    public User updateUser(@PathVariable Long id, @RequestBody User updated_user) {
        if (updated_user.getId() == null)
            updated_user.setId(id);
        else if (updated_user.getId() != id)
            throw new RuntimeException("Id does not match on the updated user");
        return service.save(updated_user);
    }

    @GetMapping("/user/get/{AuthentikUserID}")
    public List<Game> getGamesByAuthID(@PathVariable String AuthentikUserID) {
        User u = service.getUserFromAuthID(AuthentikUserID);
        List<Game> Games = new ArrayList<>();
        if (u != null) {
           var boards = u.getBoards();
           for (Board board : boards) Games.addAll(board.getGames());
        }
        return Games;
    }

}
